#include "eckit/log/CodeLocation.h"
#include "eckit/os/Backtrace.h"
#include "atlas/fortran/atlas_error.h"
#include "eckit/config/Resource.h"

namespace {
  const int atlas_err_cleared         = -1 ;
  const int atlas_err_noerr           = 0  ;
  const int atlas_err_exception       = 1  ;
  const int atlas_err_usererror       = 2  ;
  const int atlas_err_seriousbug      = 3  ;
  const int atlas_err_notimplemented  = 4  ;
  const int atlas_err_assertionfailed = 5  ;
  const int atlas_err_badparameter    = 6  ;
  const int atlas_err_outofrange      = 7  ;
  const int atlas_err_stop            = 100;
  const int atlas_err_abort           = 101;
  const int atlas_err_cancel          = 102;
  const int atlas_err_readerror       = 200;
  const int atlas_err_writeerror      = 201;
}

namespace atlas {
namespace fortran {

Error::Error()
{
  throws_    = eckit::Resource<bool>("atlas.fortran.error.throws;$ATLAS_FORTRAN_ERROR_THROWS",false);
  aborts_    = eckit::Resource<bool>("atlas.fortran.error.aborts;$ATLAS_FORTRAN_ERROR_ABORTS",false);
  backtrace_ = eckit::Resource<bool>("atlas.fortran.error.backtrace;$ATLAS_FORTRAN_ERROR_BACKTRACE",true);
  clear();
}

Error& Error::instance()
{
  static Error error_instance;
  return error_instance;
}

void Error::clear()
{
  code_      = atlas_err_cleared;
  msg_       = std::string("Error code was not set!");
}

}
}

using atlas::fortran::Error;
using eckit::CodeLocation;
using eckit::Exception;
using eckit::Log;
using eckit::BackTrace;

namespace {
void handle_exception(eckit::Exception* exception, const int err_code)
{
  std::stringstream msg;
  if( Error::instance().backtrace() || Error::instance().aborts() )
  {
    msg << "=========================================\n"
        << "ERROR\n"
        << "-----------------------------------------\n"
        << exception->what() << "\n";
    msg << "-----------------------------------------\n"
        << "BACKTRACE\n"
        << "-----------------------------------------\n"
        << exception->callStack() << "\n"
        << "=========================================";
  }
  else
  {
    msg << exception->what();
  }
  Error::instance().set_code(err_code);
  Error::instance().set_msg(msg.str());
  
  if( Error::instance().aborts() )
  {
    Log::error() << msg.str() << std::endl;
    ::abort();
  }
  if( Error::instance().throws() )
  {
    throw *exception;
  }
  delete exception;
}
}

void atlas__Error_set_aborts (int on_off)
{
  Error::instance().set_aborts(on_off);
}

void atlas__Error_set_throws (int on_off)
{
  Error::instance().set_throws(on_off);
}

void atlas__Error_set_backtrace (int on_off)
{
  Error::instance().set_backtrace(on_off);
}

void atlas__Error_success ()
{
  Error::instance().set_code(atlas_err_noerr);
  Error::instance().set_msg(std::string());
}
  
void atlas__Error_clear ()
{
  Error::instance().clear();
}

int atlas__Error_code()
{
  return Error::instance().code();
}

char* atlas__Error_msg()
{
  return const_cast<char*>(Error::instance().msg().c_str());
}

void atlas__throw_exception(char* msg, char* file, int line, char* function)
{
  eckit::Exception* exception;
  if( file && std::string(file).size() )
    exception = new eckit::Exception( std::string(msg), CodeLocation(file,line,function) );
  else
    exception = new eckit::Exception( std::string(msg) );
  handle_exception(exception,atlas_err_exception);
}

void atlas__throw_notimplemented (char* msg, char* file, int line, char* function)
{
  eckit::NotImplemented* exception;
  if( file && std::string(file).size() && std::string(msg).size() )
    exception = new eckit::NotImplemented( std::string(msg), CodeLocation(file,line,function) );
  else if( file && std::string(file).size() )
    exception = new eckit::NotImplemented( std::string(), CodeLocation(file,line,function) );
  else if( std::string(msg).size() )
    exception = new eckit::NotImplemented( std::string(msg), CodeLocation() );
  else
    exception = new eckit::NotImplemented( std::string(), CodeLocation() );
  handle_exception(exception,atlas_err_notimplemented);
}


void atlas__abort(char* msg, char* file, int line, char* function )
{
  Log::error() << "=========================================\n"
               << "ABORT\n";
  if( msg && std::string(msg).size() )
    Log::error() << "-----------------------------------------\n"
                 << msg << "\n";

  if( file && std::string(file).size() )
    Log::error() << "-----------------------------------------\n"  
                 << "LOCATION: " << CodeLocation(file,line,function) << "\n"; 
  
  Log::error() << "-----------------------------------------\n"
               << "BACKTRACE\n"
               << "-----------------------------------------\n"
               << BackTrace::dump() << "\n"
               << "========================================="
               << std::endl;
  ::abort();
}
