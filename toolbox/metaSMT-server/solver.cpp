#include "solver.hpp"

bool fd_block(int fd, bool block) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (block)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

bool is_unary(const boost::property_tree::ptree& pt)
{
    return pt.find("operand") != pt.not_found();
}

bool is_binary(const boost::property_tree::ptree& pt)
{
    return pt.find("lhs") != pt.not_found() && pt.find("rhs") != pt.not_found();
}



SolverProcess::SolverProcess(int solver_type)
{
    m_solver_type = solver_type;
    sb = 0;
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

bool SolverProcess::parent_read_command_available()
{
    fd_block(fd_c2p[0], false);

    char buf[1];

    int len = read(fd_c2p[0], buf, 1);
    bool r = len != 0 || *p2c_read_command.rbegin() == '\n';

    fd_block(fd_c2p[0], true);

    return r;
}

std::string SolverProcess::parent_read_command()
{
    std::string r;
    if (*p2c_read_command.rbegin() == '\n') {
        p2c_read_command.erase(p2c_read_command.find_last_not_of(" \n\r\t") + 1);
        r = p2c_read_command;
    } else {
        r = p2c_read_command + read_command(fd_c2p[0]);
    }
    p2c_read_command.clear();

    return r;
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
    s += '\n';
    write(fd, s.c_str(), s.length());
}

SolverProcess::~SolverProcess()
{
    delete sb;
}
