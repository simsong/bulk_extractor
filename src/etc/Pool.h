//
//  Pool.h
//  https://gist.github.com/matthewjberger/3630dd0bc07c32808431ae5ffb80a2ff

#ifndef Tools_Thread_Pool_h
#define Tools_Thread_Pool_h


// ================================================================================ Standard Includes
// Standard Includes
// --------------------------------------------------------------------------------
#include <mutex>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <future>


namespace Thread
{
    // ============================================================================ Pool
    // Pool
    //
    // A basic thread pool
    // ----------------------------------------------------------------------------
    class Pool
    {
    public:
        // -------------------------------------------------------------------- Types
        using Mutex_t          = std::recursive_mutex;
        using Unique_Lock_t    = std::unique_lock< Mutex_t >;
        using Condition_t      = std::condition_variable_any;
        using Threads_t        = std::vector< std::thread >;

        using Task_Function_t  = std::function < void()          >;
        using Task_Queue_t     = std::queue    < Task_Function_t >;


    private:
        // -------------------------------------------------------------------- State
        Threads_t    _threads;
        Task_Queue_t _task_queue;
        Mutex_t      _queue_mutex;
        Condition_t  _pool_notifier;
        bool         _should_stop_processing;
        bool         _is_emergency_stop;
        bool         _is_paused;


    public:
        // ==================================================================== Constructors / Destructors
        // Constructors / Destructors
        // -------------------------------------------------------------------- Construct ( max threads )
        Pool()

            : Pool( std::thread::hardware_concurrency() )
        {}

        // -------------------------------------------------------------------- Construct ( thread count )
        Pool( const std::size_t thread_count )

            : _should_stop_processing( false ),
              _is_emergency_stop     ( false ),
              _is_paused             ( false )
        {
            // Sanity
            if( thread_count == 0 )
                throw std::runtime_error("ERROR: Thread::Pool() -- must have at least one thread");

            // Init pool
            _threads.reserve( thread_count );

            for( std::size_t i = 0; i < thread_count; ++i )
                _threads.emplace_back( [this](){ Worker(); } );
        }

        // -------------------------------------------------------------------- Deleted
        Pool            ( const Pool & source ) = delete;
        Pool & operator=( const Pool & source ) = delete;

        // -------------------------------------------------------------------- Destruct
        ~Pool()
        {
            // Set stop flag
            {
                Unique_Lock_t queue_lock( _queue_mutex );

                _should_stop_processing = true;
            }


            // Wake up all threads and wait for them to exit
            _pool_notifier.notify_all();

            for( auto & task_thread: _threads )
                if( task_thread.joinable() )
                    task_thread.join();
        }


    public:
        // ==================================================================== Public API
        // Public API
        // -------------------------------------------------------------------- Accessors
        decltype( _threads.size() ) Thread_Count() const { return _threads.size(); }

        // -------------------------------------------------------------------- Add_Simple_Task()
        template < typename Lambda_t >

        void Add_Simple_Task( Lambda_t && function )
        {
            // Add to task queue
            {
                Unique_Lock_t queue_lock( _queue_mutex );

                // Sanity
                if( _should_stop_processing || _is_emergency_stop )
                    throw std::runtime_error( "ERROR: Thread::Pool::Add_Simple_Task() - attempted to add task to stopped pool" );

                // Add our task to the queue
                _task_queue.emplace( std::forward< Lambda_t >(function) );
            }

            // Notify the pool that there is work to do
            _pool_notifier.notify_one();
        }

        // -------------------------------------------------------------------- Add_Task()
        template < typename    Function_t,
                   typename... Args >

        auto Add_Task( Function_t &&    function,
                       Args       &&... args )

            -> std::future< typename std::result_of< Function_t(Args...) >::type >
        {
            // Types
            using return_t = typename std::result_of< Function_t(Args...) >::type;

            // Create packaged task
            auto task = std::make_shared< std::packaged_task< return_t() > >
            (
                std::bind
                (
                    std::forward< Function_t >( function ),
                    std::forward< Args       >( args     )...
                )
            );

            std::future< return_t > result = task->get_future();


            // Add task to queue
            {
                Unique_Lock_t queue_lock( _queue_mutex );

                // Sanity
                if( _should_stop_processing || _is_emergency_stop )
                    throw std::runtime_error( "ERROR: Thread::Pool::Add_Task() - attempted to add task to stopped pool" );

                // Add our task to the queue
                _task_queue.emplace( [task](){ (*task)(); } );
            }


            // Notify the pool that there is work to do
            _pool_notifier.notify_one();

            return result;
        };


        // -------------------------------------------------------------------- Emergency_Stop()
        void Emergency_Stop()
        {
            {
                Unique_Lock_t queue_lock( _queue_mutex );

                _is_emergency_stop = true;
            }

            _pool_notifier.notify_all();
        }

        // -------------------------------------------------------------------- Pause()
        void Pause( bool pause_state )
        {
            {
                Unique_Lock_t queue_lock( _queue_mutex );

                _is_paused = pause_state;
            }

            _pool_notifier.notify_all();
        }


    public:
        // ==================================================================== Private API
        // Private API
        // -------------------------------------------------------------------- Worker()
        void Worker()
        {
            while( true )
            {
                Task_Function_t task;

                // Scoped waiting / task-retrieval block
                {
                    // Wait on tasks or 'stop processing' flags
                    Unique_Lock_t queue_lock( _queue_mutex );

                    _pool_notifier.wait
                    (
                        queue_lock,

                        [this]()
                        { return   (!_task_queue.empty() && !_is_paused)
                                 ||  _should_stop_processing
                                 ||  _is_emergency_stop; }
                    );


                    // Bail when stopped and no more tasks remain,
                    // or if an emergency stop has been requested.
                    if(    (_should_stop_processing && _task_queue.empty())
                        ||  _is_emergency_stop )
                        return;

                    // Retrieve next task
                    task = std::move( _task_queue.front() );
                    _task_queue.pop();
                }

                // Execute task
                task();
            }
        }
    };
}

#endif
