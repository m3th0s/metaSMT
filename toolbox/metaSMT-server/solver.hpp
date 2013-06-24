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

bool is_unary(const boost::property_tree::ptree& pt);
bool is_binary(const boost::property_tree::ptree& pt);
//std::string next_line(socket_ptr socket, boost::asio::streambuf& buffer);
void new_connection(socket_ptr socket);


class UnsupportedOperatorException : public std::runtime_error
{
public:
    UnsupportedOperatorException(std::string op) : std::runtime_error(op) {}
};

class UnsupportedCommandException : public std::runtime_error
{
public:
    UnsupportedCommandException(std::string command) : std::runtime_error(command) {}
};

class UndefinedVariableException : public std::runtime_error
{
public:
    UndefinedVariableException(std::string variable) : std::runtime_error(variable) {}
};


template<typename Context, typename Predicates, typename Bitvectors>
typename Context::result_type create_unary_assertion(Context& ctx, const Predicates& predicates, const Bitvectors& bitvectors, const boost::property_tree::ptree& pt);

template<typename Context, typename Predicates, typename Bitvectors>
typename Context::result_type create_binary_assertion(Context& ctx, const Predicates& predicates, const Bitvectors& bitvectors, const boost::property_tree::ptree& pt);

template<typename Context, typename Predicates, typename Bitvectors>
typename Context::result_type create_assertion(Context& ctx, const Predicates& predicates, const Bitvectors& bitvectors, const boost::property_tree::ptree& pt)
{
    std::string op = pt.get<std::string>("op");

    if (is_binary(pt)) {
        return create_binary_assertion(ctx, predicates, bitvectors, pt);
    } else if (is_unary(pt)) {
        return create_unary_assertion(ctx, predicates, bitvectors, pt);
    } else if (op == "variable") {
        std::string name = pt.get<std::string>("name");

        typename Predicates::const_iterator itp = predicates.find(name);
        if (itp != predicates.end()) {
            return evaluate(ctx, itp->second);
        } else {
            typename Bitvectors::const_iterator itb = bitvectors.find(name);
            if (itb != bitvectors.end()) {
                return evaluate(ctx, itb->second);
            } else {
                throw(UndefinedVariableException(name));
            }
        }
    } else if (op == "integer") {
//        std::cout << pt.get<signed>("value") << std::endl;
        return evaluate(ctx, metaSMT::logic::QF_BV::bvuint(pt.get<signed>("value"), pt.get<unsigned>("width")));
    } else {
        throw(UnsupportedOperatorException(op));
    }
}

template<typename Context, typename Predicates, typename Bitvectors>
typename Context::result_type create_unary_assertion(Context& ctx, const Predicates& predicates, const Bitvectors& bitvectors, const boost::property_tree::ptree& pt)
{
    std::string op = pt.get<std::string>("op");

    typename Context::result_type operand = create_assertion(ctx, predicates, bitvectors, pt.get_child("operand"));
    if (op == "not") {
        return evaluate(ctx, metaSMT::logic::Not(operand));
    }

    else if (op == "bvnot") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvnot(operand));
    } else if (op == "bvneg") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvneg(operand));
    } else {
        throw(UnsupportedOperatorException(op));
    }
}

template<typename Context, typename Predicates, typename Bitvectors>
typename Context::result_type create_binary_assertion(Context& ctx, const Predicates& predicates, const Bitvectors& bitvectors, const boost::property_tree::ptree& pt)
{
    std::string op = pt.get<std::string>("op");

    typename Context::result_type lhs = create_assertion(ctx, predicates, bitvectors, pt.get_child("lhs"));
    typename Context::result_type rhs = create_assertion(ctx, predicates, bitvectors, pt.get_child("rhs"));

    if (op == "equal" || op == "=") {
        return evaluate(ctx, metaSMT::logic::equal(lhs, rhs));
    } else if (op == "nequal" || op == "!=") {
        return evaluate(ctx, metaSMT::logic::nequal(lhs, rhs));
    } else if (op == "implies" || op == "=>") {
        return evaluate(ctx, metaSMT::logic::implies(lhs, rhs));
    } else if (op == "and" || op == "&&") {
        return evaluate(ctx, metaSMT::logic::And(lhs, rhs));
    } else if (op == "nand") {
        return evaluate(ctx, metaSMT::logic::Nand(lhs, rhs));
    } else if (op == "or" || op == "||") {
        return evaluate(ctx, metaSMT::logic::Or(lhs, rhs));
    } else if (op == "nor") {
        return evaluate(ctx, metaSMT::logic::Nor(lhs, rhs));
    } else if (op == "xor") {
        return evaluate(ctx, metaSMT::logic::Xor(lhs, rhs));
    } else if (op == "xnor") {
        return evaluate(ctx, metaSMT::logic::Xnor(lhs, rhs));
    }


    else if (op == "bvand" || op == "&") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvand(lhs, rhs));
    } else if (op == "bvnand") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvnand(lhs, rhs));
    } else if (op == "bvor" || op == "|") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvor(lhs, rhs));
    } else if (op == "bvnor") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvnor(lhs, rhs));
    } else if (op == "bvxor" || op == "^") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvxor(lhs, rhs));
    } else if (op == "bvxnor") {
      return evaluate(ctx, metaSMT::logic::QF_BV::bvxnor(lhs, rhs));

    } else if (op == "bvadd" || op == "+") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvadd(lhs, rhs));
    } else if (op == "bvmul" || op == "*") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvmul(lhs, rhs));
    } else if (op == "bvsub" || op == "-") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvsub(lhs, rhs));
    } else if (op == "bvudiv") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvudiv(lhs, rhs));
    } else if (op == "bvurem") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvurem(lhs, rhs));
    } else if (op == "bvsdiv") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvsdiv(lhs, rhs));
    } else if (op == "bvsrem") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvsrem(lhs, rhs));

    } else if (op == "bvcomp" || op == "==") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvcomp(lhs, rhs));

    } else if (op == "bvslt") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvslt(lhs, rhs));
    } else if (op == "bvsgt") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvsgt(lhs, rhs));
    } else if (op == "bvsle") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvsle(lhs, rhs));
    } else if (op == "bvsge") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvsge(lhs, rhs));

    } else if (op == "bvult") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvult(lhs, rhs));
    } else if (op == "bvugt") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvugt(lhs, rhs));
    } else if (op == "bvule") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvule(lhs, rhs));
    } else if (op == "bvuge") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvuge(lhs, rhs));
    } else if (op == "bvshl") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvshl(lhs, rhs));
    } else if (op == "bvshr") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvshr(lhs, rhs));
    } else if (op == "bvashr") {
        return evaluate(ctx, metaSMT::logic::QF_BV::bvashr(lhs, rhs));

    } else if (op == "concat" || op == "++") {
        return evaluate(ctx, metaSMT::logic::QF_BV::concat(lhs, rhs));
    } else {
        throw(UnsupportedOperatorException(op));
    }
}

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
