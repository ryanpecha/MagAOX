/** \file logMap.cpp
 * \brief Declares and defines the logMap class and related classes.
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 * \ingroup logger_files
 *
 */

#include "logMap.hpp"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <limits>

#include <mx/mxException.hpp>

#include "../common/exceptions.hpp"

using namespace flatlogs;

namespace MagAOX
{
namespace logger
{

int logInMemory::loadFile( file::stdFileName<verboseT> const &lfn )
{
    DEBUG_CRUMB( "logInMemory loadFile begin file=" + lfn.fullName() );

    int fd = open( lfn.fullName().c_str(), O_RDONLY );

    off_t fsz = mx::ioutils::fileSize( fd );
    if( fsz <= 0 )
    {
        close( fd );
        std::cerr << "logInMemory::loadFile(" << lfn.fullName() << ") is empty\n";
        return -1;
    }

    std::vector<char> memory( fsz );

    ssize_t nrd = read( fd, memory.data(), memory.size() );

    close( fd );

    if( nrd != fsz )
    {
        std::cerr << "logInMemory::loadFile(" << lfn.fullName() << ") did not read all bytes\n";
        return -1;
    }

    flatlogs::timespecX startTime = logHeader::timespec( memory.data() );

    size_t st = 0;
    size_t ed = logHeader::totalSize( memory.data() );
    st        = ed;

    while( st < memory.size() )
    {
        ed = logHeader::totalSize( memory.data() + st );
        st = st + ed;
    }

    if( st != memory.size() )
    {
        std::cerr << "Possibly corrupt logfile.\n";
        return -1;
    }

    st -= ed;

    flatlogs::timespecX endTime = logHeader::timespec( memory.data() + st );

    DEBUG_CRUMB( "logInMemory loadFile read file=" + lfn.fullName() + " bytes=" + std::to_string( memory.size() ) +
                 " start=" + logMapDebugTime( startTime ) + " end=" + logMapDebugTime( endTime ) );

    if( m_memory.size() == 0 )
    {
        m_memory.swap( memory );
        m_startTime = startTime;
        m_endTime   = endTime;
        m_loadedFiles.push_back( { 0, m_memory.size(), lfn.fullName() } );

        DEBUG_CRUMB( "logInMemory loadFile initial file=" + lfn.fullName() +
                     " loadedStart=" + logMapDebugTime( m_startTime ) + " loadedEnd=" + logMapDebugTime( m_endTime ) +
                     " memoryBytes=" + std::to_string( m_memory.size() ) );

        return 0;
    }

    if( startTime < m_startTime )
    {

        if( endTime >= m_startTime )
        {
            std::cerr << "overlapping log files!\n";
            return -1;
        }

        m_memory.insert( m_memory.begin(), memory.begin(), memory.end() );
        for( loadedFile &loaded : m_loadedFiles )
        {
            loaded.m_begin += memory.size();
            loaded.m_end += memory.size();
        }
        m_loadedFiles.insert( m_loadedFiles.begin(), { 0, memory.size(), lfn.fullName() } );
        m_startTime = startTime;
        DEBUG_CRUMB( "logInMemory loadFile prepended file=" + lfn.fullName() +
                     " loadedStart=" + logMapDebugTime( m_startTime ) + " loadedEnd=" + logMapDebugTime( m_endTime ) +
                     " memoryBytes=" + std::to_string( m_memory.size() ) );
        return 0;
    }

    if( startTime > m_endTime )
    {
        size_t loadedBegin = m_memory.size();
        m_memory.insert( m_memory.end(), memory.begin(), memory.end() );
        m_loadedFiles.push_back( { loadedBegin, m_memory.size(), lfn.fullName() } );
        m_endTime = endTime;

        DEBUG_CRUMB( "logInMemory loadFile appended file=" + lfn.fullName() +
                     " loadedStart=" + logMapDebugTime( m_startTime ) + " loadedEnd=" + logMapDebugTime( m_endTime ) +
                     " memoryBytes=" + std::to_string( m_memory.size() ) );

        return 0;
    }

    std::cerr << "Need to implement insert in the middle!\n";
    std::cerr << m_startTime.time_s << " " << m_startTime.time_ns << "\n";
    std::cerr << startTime.time_s << " " << startTime.time_ns << "\n";

    return -1;
}

std::string logInMemory::sourceFile( char *log ) const
{
    if( log == nullptr || m_memory.empty() )
    {
        return "<unknown>";
    }

    const char *bufferStart = m_memory.data();
    const char *bufferEnd   = m_memory.data() + m_memory.size();
    if( log < bufferStart || log >= bufferEnd )
    {
        return "<outside-loaded-buffer>";
    }

    size_t offset = static_cast<size_t>( log - bufferStart );
    for( const loadedFile &loaded : m_loadedFiles )
    {
        if( offset >= loaded.m_begin && offset < loaded.m_end )
        {
            return loaded.m_name;
        }
    }

    return "<unknown-loaded-file>";
}

size_t logInMemory::sourceOffset( char *log ) const
{
    if( log == nullptr || m_memory.empty() )
    {
        return std::numeric_limits<size_t>::max();
    }

    const char *bufferStart = m_memory.data();
    const char *bufferEnd   = m_memory.data() + m_memory.size();
    if( log < bufferStart || log >= bufferEnd )
    {
        return std::numeric_limits<size_t>::max();
    }

    size_t offset = static_cast<size_t>( log - bufferStart );
    for( const loadedFile &loaded : m_loadedFiles )
    {
        if( offset >= loaded.m_begin && offset < loaded.m_end )
        {
            return offset - loaded.m_begin;
        }
    }

    return std::numeric_limits<size_t>::max();
}

template class logMap<XWC_DEFAULT_VERBOSITY>;

} // namespace logger
} // namespace MagAOX
