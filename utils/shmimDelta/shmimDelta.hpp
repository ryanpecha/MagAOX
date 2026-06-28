/** \file shmimDelta.hpp
 * \brief The shmimDelta class declaration.
 *
 * \ingroup shmimDelta_files
 *
 * \author Codex
 */

#ifndef shmimDelta_hpp
#define shmimDelta_hpp

#include <ImageStreamIO/ImageStruct.h>
#include <ImageStreamIO/ImageStreamIO.h>

#include <semaphore.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <vector>

#include "../../libMagAOX/libMagAOX.hpp"

/** \defgroup shmimDelta shmimDelta: measure semaphore-arrival deltas between two shmims
 * \brief Attaches to two shmims and reports the mean and rms delta between their semaphore arrivals.
 *
 * \ingroup utils
 *
 */

/** \defgroup shmimDelta_files shmimDelta Files
 * \ingroup shmimDelta
 */

/// Utility for measuring the delta between two shmim semaphore arrival times.
/**
 * \ingroup shmimDelta
 */
class shmimDelta : public mx::app::application
{
  protected:
    /// Runtime state for one monitored shmim.
    struct streamState
    {
        std::string m_name; ///< Name of the shmim to monitor.

        IMAGE m_imageStream{}; ///< Attached ImageStreamIO handle for this shmim.

        bool m_opened{ false }; ///< Tracks whether `m_imageStream` owns an open attachment.

        ino_t m_inode{ 0 }; ///< Inode of the backing shmim file used to detect stream replacement.

        int m_semaphoreNumber{ 9 }; ///< Preferred semaphore index used to subscribe to frame arrivals.

        sem_t *m_semaphore{ nullptr }; ///< Semaphore selected by ImageStreamIO for this reader.

        uint32_t m_width{ 0 }; ///< Stream size along the first axis.

        uint32_t m_height{ 1 }; ///< Stream size along the second axis, or 1 when absent.

        uint32_t m_depth{ 1 }; ///< Stream size along the third axis, or 1 when absent.

        std::vector<timespec> m_arrivalTimes; ///< Local timestamps recorded after semaphore wakes.

        std::string m_errorMessage; ///< Error text recorded by the waiter thread on failure.

        int m_status{ 0 }; ///< Waiter-thread completion status, 0 on success and -1 on error.
    };

    /** \name Configurable Parameters
     * @{
     */

    std::string m_shmimName1; ///< Name of the first shmim, used as the reference arrival time.

    std::string m_shmimName2; ///< Name of the second shmim. Reported delta is shmimName2 minus shmimName1.

    size_t m_nFrames{ 100 }; ///< Number of paired semaphore arrivals to measure.

    double m_timeoutSec{ 1.0 }; ///< Timeout, in seconds, used for each frame-arrival wait.

    ///@}

    /** \name Stream State - Data
     * @{
     */

    streamState m_stream1; ///< State for `m_shmimName1`.

    streamState m_stream2; ///< State for `m_shmimName2`.

    ///@}

    /** \name Measurement - Data
     * @{
     */

    std::vector<double> m_deltaSeconds; ///< Paired semaphore-arrival deltas in seconds.

    ///@}

  public:
    /// Default constructor.
    shmimDelta();

    /// Destructor.
    ~shmimDelta() override;

    /// Define command-line and config-file options.
    void setupConfig() override;

    /// Load configured values into member state.
    void loadConfig() override;

    /// Connect to both shmims, collect semaphore arrivals, and report delta statistics.
    int execute() override;

    /// Convert a pair of timespec timestamps to elapsed seconds.
    static double elapsedSeconds( const timespec &t0 /**< [in] starting timestamp. */,
                                  const timespec &t1 /**< [in] ending timestamp. */ );

    /// Calculate the arithmetic mean of a vector.
    static double mean( const std::vector<double> &values /**< [in] values to average. */ );

    /// Calculate the rms scatter about a supplied mean.
    static double rms( const std::vector<double> &values /**< [in] values to calculate rms for. */,
                       double                     meanValue /**< [in] arithmetic mean of `values`. */ );

  protected:
    /// Open one shmim and cache its dimensions.
    int openStream( streamState &stream /**< [in,out] stream state to open. */ );

    /// Close one shmim attachment if it is open.
    void closeStream( streamState &stream /**< [in,out] stream state to close. */ );

    /// Close both shmim attachments if open.
    void closeStreams();

    /// Select and flush the semaphore used to monitor one stream.
    int prepareSemaphore( streamState &stream /**< [in,out] stream state to prepare. */ );

    /// Collect semaphore-arrival timestamps from both streams and report statistics.
    int measureDeltas();

    /// Thread entry point for collecting arrival timestamps from one stream.
    static void waitThreadStart( shmimDelta  *app /**< [in] owning application instance. */,
                                 streamState *stream /**< [in,out] stream state to monitor. */ );

    /// Collect all configured frame-arrival timestamps for one stream.
    int waitFrames( streamState &stream /**< [in,out] stream state to monitor. */ );

    /// Wait for one frame arrival and record its timestamp.
    int waitForFrame( streamState &stream /**< [in,out] stream state to monitor. */,
                      size_t       frameNumber /**< [in] one-based frame number used for error reporting. */ );

    /// Check whether a stream disappeared or was replaced after a wait error.
    bool streamChanged( const streamState &stream /**< [in] stream state to check. */ ) const;

    /// Fill a timespec with the absolute timeout deadline for sem_timedwait.
    int setWaitDeadline( timespec &deadline /**< [out] absolute realtime wait deadline. */ ) const;
};

#endif // shmimDelta_hpp
