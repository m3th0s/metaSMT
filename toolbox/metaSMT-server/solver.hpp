#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <map>

#include <metaSMT/support/default_visitation_unrolling_limit.hpp>
#include <metaSMT/DirectSolver_Context.hpp>
#include <metaSMT/support/parser/UTreeEvaluator.hpp>
#include <metaSMT/support/parser/SMT2Parser.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <sstream>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <boost/lexical_cast.hpp>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

class SolverBase;

class SolverProcess
{
public:
    SolverProcess(int solver_type);
    ~SolverProcess();
    int initPipes();

    std::string child_read_command();
    void child_write_command(std::string s);

    bool parent_read_command_available();
    std::string parent_read_command();
    void parent_write_command(std::string s);

    SolverBase* sb;
    int m_solver_type;
    pid_t pid;
private:
    int fd_p2c[2];
    int fd_c2p[2];

    std::string p2c_read_command;

    std::string read_command(int fd);
    void write_command(int fd, std::string s);
};

class SolverBase
{
public:
    virtual void start() = 0;
    virtual ~SolverBase() {};
// protected:
    SolverProcess* sp;
};

template<typename Context> class Solver : public SolverBase
{
public:

    Solver() :
        evaluator(solver),
        parser(evaluator)
    {}

    void start()
    {
        std::string ret;
        std::stringstream buf;
        while (true) {
                ret = "OK";
                std::string line = sp->child_read_command();
                buf << line << std::endl;

                size_t found = line.find("(get-value");
                if(line.compare("(check-sat)") == 0 || found != line.npos){
                    boost::spirit::utree::list_type ast;
                    parser.parse(buf, ast);

                    ret = evaluator.evaluateInstance(ast);
                }
                sp->child_write_command(ret);
        }
    }
private:
    metaSMT::DirectSolver_Context<Context> solver;
    metaSMT::evaluator::UTreeEvaluator<metaSMT::DirectSolver_Context<Context> > evaluator;
    metaSMT::smt2::SMT2Parser<metaSMT::evaluator::UTreeEvaluator<metaSMT::DirectSolver_Context<Context> > > parser;

    std::map<std::string, metaSMT::logic::predicate> predicates;
    std::map<std::string, metaSMT::logic::QF_BV::bitvector> bitvectors;
};

#endif
