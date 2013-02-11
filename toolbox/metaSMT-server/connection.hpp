#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <map>

#include <boost/asio/ip/tcp.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/property_tree/ptree.hpp>

#include <metaSMT/DirectSolver_Context.hpp>
#include <metaSMT/backend/Z3_Backend.hpp>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

class Connection
{
public:
    Connection(socket_ptr socket);
    void start();

private:
    socket_ptr sock;

    metaSMT::DirectSolver_Context<metaSMT::solver::Z3_Backend> solver;
    std::map<std::string, metaSMT::logic::predicate> predicates;
    std::map<std::string, metaSMT::logic::QF_BV::bitvector> bitvectors;

    template<typename Context, typename Bitvectors>
        typename Context::result_type create_assertion(Context& ctx, const Bitvectors& bitvectors, const boost::property_tree::ptree& pt);
    bool isBinaryX(const boost::property_tree::ptree& pt);
    bool isBinary(const boost::property_tree::ptree& pt);

};

#endif
