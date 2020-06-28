#ifndef CONEXIONADMIN_H_
#define CONEXIONADMIN_H_

#include <boost/cstdint.hpp>
#include <vector>
#include <sys/select.h>

#include <ConexionActiva.h>

namespace ZeusServicioAdmin
{

class ConexionAdmin : public ConexionActiva
{
public:    
    ConexionAdmin();
    virtual ~ConexionAdmin();
    
    void dipatch(std::vector<boost::uint8_t> const& buffer);
        
    void dipatchTask(std::vector<boost::uint8_t> const& vectorBuffer);    
    
    bool isConnected() const;
    
    void close();
    
    bool isSet(fd_set* set) const;
    
    std::vector<boost::uint8_t> getMessage();
    
    std::string getIpAddress() const;
    
    static std::auto_ptr<ConexionAdmin> accept(int socket);
    
    void sendClose();
    
    int getSd() const;
};

}

#endif /*CONEXIONADMIN_H_*/
