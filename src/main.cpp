//! See https://isatec.ca/fastcgipp/helloWorld.html
//! [Request definition]
#include <fastcgi++/request.hpp>
//! [Request definition]
#include <fastcgi++/request.hpp>

class HelloWorld: public Fastcgipp::Request<wchar_t>
{
    //! [Request definition]
    //! [Response definition]
    bool response()
    {
        out << "<html><head></head><body></body></html>"
        return true;
    }
};
//! [Return]

//! [Manager]
#include <fastcgi++/manager.hpp>

int main()
{
    Fastcgipp::Manager<HelloWorld> manager;
    //! [Manager]
    //! [Signals]
    manager.setupSignals();
    //! [Signals]
    //! [Listen]
    manager.listen();
    //! [Listen]
    //! [Start]
    manager.start();
    //! [Start]
    //! [Join]
    manager.join();

    return 0;
}
