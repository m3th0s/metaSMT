#include "connection.hpp"

#include <signal.h>
#include <sys/wait.h>

#include <list>

#include <boost/thread/thread.hpp>

#include <metaSMT/BitBlast.hpp>
#include <metaSMT/backend/Boolector.hpp>
#include <metaSMT/backend/SAT_Clause.hpp>
#include <metaSMT/backend/PicoSAT.hpp>
#include <metaSMT/backend/Z3_Backend.hpp>

SolverProcess::SolverProcess(int solver_type)
{
    m_solver_type = solver_type;
}

int SolverProcess::initPipes()
{
    return pipe(fd_c2p) != -1 && pipe(fd_p2c) != -1;
}

std::string SolverProcess::child_read_command()
{
    return read_command(fd_p2c[0]);
}

void SolverProcess::child_write_command(std::string s)
{
    write_command(fd_c2p[1], s);
}

std::string SolverProcess::parent_read_command()
{
    return read_command(fd_c2p[0]);
}

void SolverProcess::parent_write_command(std::string s)
{
    write_command(fd_p2c[1], s);
}

std::string SolverProcess::read_command(int fd)
{
    std::string s;
    char buf[1];
    do {
        read(fd, buf, 1);
        s += buf[0];
    } while (buf[0] != '\n');

    s.erase(s.find_last_not_of(" \n\r\t") + 1);
    return s;
}

void SolverProcess::write_command(int fd, std::string s)
{
    s += "\n";
    write(fd, s.c_str(), s.length());
}

bool is_unary(const boost::property_tree::ptree& pt)
{
    return pt.find("operand") != pt.not_found();
}

bool is_binary(const boost::property_tree::ptree& pt)
{
    return pt.find("lhs") != pt.not_found() && pt.find("rhs") != pt.not_found();
}

std::string next_line(socket_ptr socket, boost::asio::streambuf& buffer)
{
    boost::system::error_code error;
    size_t length = read_until(*socket, buffer, '\n', error);
    //if (error == boost::asio::error::eof) break;
    /*else*/ if (error) throw boost::system::system_error(error);

    std::istream is(&buffer);
    std::string s;
    std::getline(is, s);
    s.erase(s.find_last_not_of(" \n\r\t") + 1);

    return s;
}

void new_connection(socket_ptr socket)
{
    std::cout << "New connection" << std::endl;

    try {
        std::string availableSolvers = "0 z3; 1 picosat; 2 boolector\n";
        boost::asio::write(*socket, boost::asio::buffer(availableSolvers, availableSolvers.size()));

        boost::asio::streambuf buffer;
        std::string str;
        std::string ret;

        std::list<SolverProcess*> solvers;

        //select solvers
        while (true) {
            ret = "OK\n";
            str = next_line(socket, buffer);
            int solver;
            try {
                solver = boost::lexical_cast<unsigned>(str);
            } catch (boost::bad_lexical_cast e) {
                break;
            }

            if (0 <= solver && solver <= 2)
                solvers.push_back(new SolverProcess(solver));
            else
                ret = "FAIL unsupported solver\n";

            boost::asio::write(*socket, boost::asio::buffer(ret, ret.size()));
        }

        //receive commands
        for (std::list<SolverProcess*>::iterator i = solvers.begin(); i != solvers.end(); i++) {
            if (!(*i)->initPipes()) {
                ret = "Could not create pipe for IPC.\n";
                std::cout << ret << std::endl;
                boost::asio::write(*socket, boost::asio::buffer(ret, ret.size()));
                return;
            }
            pid_t pid = fork();
            if (pid == -1) {
                ret = "Could not fork new process.\n";
                std::cout << ret << std::endl;
                boost::asio::write(*socket, boost::asio::buffer(ret, ret.size()));
                return;
            } else if (pid) {
                //PARENT PROCESS
                (*i)->pid = pid;
            } else {
                //CHILD PROCESS
                switch ((*i)->m_solver_type) {
                case 0:
                    (*i)->cb = new Connection<metaSMT::solver::Z3_Backend>();
                    break;
                case 1:
                    (*i)->cb = new Connection<metaSMT::BitBlast<metaSMT::SAT_Clause<metaSMT::solver::PicoSAT> > >();
                    break;
                case 2:
                    (*i)->cb = new Connection<metaSMT::solver::Boolector>();
                }

                (*i)->cb->sp = (*i);
                (*i)->cb->start();
                return;
            }
        }

        while (true) {
            if (str == "exit") {
                for (std::list<SolverProcess*>::iterator i = solvers.begin(); i != solvers.end(); i++) {
                    kill((*i)->pid, SIGTERM);
                    std::cout << "fuu" << std::endl;
                    waitpid((*i)->pid, 0, 0);
                }
                return;
            }

            for (std::list<SolverProcess*>::iterator i = solvers.begin(); i != solvers.end(); i++) {
                (*i)->parent_write_command(str);
            }

            std::vector<std::string> answers(solvers.size());
            int n = 0;
            for (std::list<SolverProcess*>::iterator i = solvers.begin(); i != solvers.end(); i++) {
                answers[n++] = (*i)->parent_read_command();
            }

            //return a FAIL if not all answers are the same, otherwise return the consistent answer
            ret = answers[0] + "\n";
            for (int n = 0; n < answers.size() -1; n++) {
                if (answers[n] != answers[n+1]) {
                    ret = "FAIL inconsistent solver behavior\n";
                    break;
                }
            }

            boost::asio::write(*socket, boost::asio::buffer(ret, ret.size()));
            str = next_line(socket, buffer);
        }
    } catch (std::exception& e) {
        std::cout << "Closing connection: " << e.what() << std::endl;
        socket->close();
        return;
    }
}
