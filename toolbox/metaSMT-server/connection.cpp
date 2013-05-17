#include "connection.hpp"

#include <signal.h>
#include <sys/wait.h>

#include <metaSMT/BitBlast.hpp>
#include <metaSMT/backend/Boolector.hpp>
#include <metaSMT/backend/SAT_Clause.hpp>
#include <metaSMT/backend/PicoSAT.hpp>
#include <metaSMT/backend/Z3_Backend.hpp>

std::string Connection::next_line()
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

void Connection::write(std::string string)
{
    string += '\n';
    boost::asio::write(*socket, boost::asio::buffer(string, string.size()));
}

void terminate_solvers()
{
    
}

void Connection::new_connection(socket_ptr socket)
{
    std::cout << "New connection" << std::endl;
    Connection c(socket);
}

Connection::Connection(socket_ptr socket) :
    socket(socket)
{
    try {
        std::string availableSolvers = "0 z3; 1 picosat; 2 boolector";
        write(availableSolvers);

        std::string str;
        std::string ret;

        //select solvers
        while (true) {
            ret = "OK";
            str = next_line();
            int solver;
            try {
                solver = boost::lexical_cast<unsigned>(str);
            } catch (boost::bad_lexical_cast e) {
                break;
            }

            if (0 <= solver && solver <= 2)
                solvers.push_back(new SolverProcess(solver));
            else
                ret = "FAIL unsupported solver";

            write(ret);
        }

        //receive commands
        for (std::list<SolverProcess*>::iterator i = solvers.begin(); i != solvers.end(); i++) {
            if (!(*i)->initPipes()) {
                ret = "Could not create pipe for IPC.";
                std::cout << ret << std::endl;
                write(ret);
                return;
            }
            pid_t pid = fork();
            if (pid == -1) {
                ret = "Could not fork new process.";
                std::cout << ret << std::endl;
                write(ret);
                return;
            } else if (pid) {
                //PARENT PROCESS
                (*i)->pid = pid;
            } else {
                //CHILD PROCESS
                switch ((*i)->m_solver_type) {
                case 0:
                    (*i)->cb = new Solver<metaSMT::solver::Z3_Backend>();
                    break;
                case 1:
                    (*i)->cb = new Solver<metaSMT::BitBlast<metaSMT::SAT_Clause<metaSMT::solver::PicoSAT> > >();
                    break;
                case 2:
                    (*i)->cb = new Solver<metaSMT::solver::Boolector>();
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
            ret = answers[0];
            for (int n = 0; n < answers.size() -1; n++) {
                if (answers[n] != answers[n+1]) {
                    ret = "FAIL inconsistent solver behavior";
                    break;
                }
            }

            write(ret);
            str = next_line();
        }
    } catch (std::exception& e) {
        std::cout << "Closing connection: " << e.what() << std::endl;
        socket->close();
        return;
    }
}
