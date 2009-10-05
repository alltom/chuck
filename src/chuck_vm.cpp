/*----------------------------------------------------------------------------
    ChucK Concurrent, On-the-fly Audio Programming Language
      Compiler and Virtual Machine

    Copyright (c) 2004 Ge Wang and Perry R. Cook.  All rights reserved.
      http://chuck.cs.princeton.edu/
      http://soundlab.cs.princeton.edu/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
    U.S.A.
-----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
// file: chuck_vm.cpp
// desc: ...
//
// authors: Ge Wang (gewang@cs.princeton.edu)
//          Perry R. Cook (prc@cs.princeton.edu)
// date: Autumn 2002
//-----------------------------------------------------------------------------
#include "chuck_vm.h"
#include "chuck_instr.h"
#include "chuck_bbq.h"
#include "chuck_errmsg.h"
#include "chuck_dl.h"
#include "chuck_type.h"
#include "chuck_globals.h"
#include "chuck_lang.h"
#include "ugen_xxx.h"

#include <algorithm>
using namespace std;

#if defined(__PLATFORM_WIN32__)
  #include <windows.h>
#else
  #include <unistd.h>
  #include <pthread.h>
#endif




//-----------------------------------------------------------------------------
// name: struct Chuck_VM_Frame
// desc: func frame
//-----------------------------------------------------------------------------
struct Chuck_VM_Frame
{
public:
    t_CKUINT size;

public:
    Chuck_VM_Frame() { size = 0; }
    ~Chuck_VM_Frame() { }
};




//-----------------------------------------------------------------------------
// name: struct Chuck_VM_Func
// desc: vm function
//-----------------------------------------------------------------------------
struct Chuck_VM_Func
{
public:
    Chuck_VM_Code * code;
    Chuck_VM_Frame * frame;
    t_CKUINT index;

public:
    Chuck_VM_Func() { code = NULL; frame = NULL; index = 0; }
    ~Chuck_VM_Func() { }
};




//-----------------------------------------------------------------------------
// name: struct Chuck_VM_FTable
// desc: function table
//-----------------------------------------------------------------------------
struct Chuck_VM_FTable
{
public:
    Chuck_VM_FTable();
    ~Chuck_VM_FTable();

public:
    Chuck_VM_Func * get_func( t_CKUINT index );
    t_CKUINT append( Chuck_VM_Func * f );
    t_CKBOOL remove( t_CKUINT index );

public:
    vector<Chuck_VM_Func *> func_table;
};




//-----------------------------------------------------------------------------
// name: Chuck_VM()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM::Chuck_VM()
{
    m_shreds = NULL;
    m_num_shreds = 0;
    m_shreduler = NULL;
    m_num_dumped_shreds = 0;
    m_msg_buffer = NULL;
    m_reply_buffer = NULL;
    m_event_buffer = NULL;
    m_shred_id = 0;
    m_halt = TRUE;
    m_audio = FALSE;
    m_block = TRUE;
    m_running = FALSE;

    m_audio_started = FALSE;
    m_dac = NULL;
    m_adc = NULL;
    m_bunghole = NULL;
    m_num_dac_channels = 0;
    m_num_adc_channels = 0;
    m_init = FALSE;
}




//-----------------------------------------------------------------------------
// name: ~Chuck_VM()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM::~Chuck_VM()
{
    if( m_init ) shutdown();
}




// dac tick
//UGEN_TICK __dac_tick( Chuck_Object * SELF, SAMPLE in, SAMPLE * out ) 
//{ *out = in; return TRUE; }
//UGEN_TICK __bunghole_tick( Chuck_Object * SELF, SAMPLE in, SAMPLE * out )
//{ *out = 0.0f; return TRUE; }


// static
#ifdef __MACOSX_CORE__
t_CKINT Chuck_VM::our_priority = 85;
#else
t_CKINT Chuck_VM::our_priority = 0x7fffffff;
#endif


#if !defined(__PLATFORM_WIN32__) || defined(__WINDOWS_PTHREAD__)
//-----------------------------------------------------------------------------
// name: set_priority()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::set_priority( t_CKINT priority, Chuck_VM * vm )
{
    struct sched_param param;
    pthread_t tid = pthread_self();
    int policy;

    // log
    EM_log( CK_LOG_INFO, "setting thread priority to: %ld...", priority );

    // get for thread
    if( pthread_getschedparam( tid, &policy, &param) ) 
    {
        if( vm )
            vm->m_last_error = "could not get current scheduling parameters";
        return FALSE;
    }

    // priority
    param.sched_priority = priority;
    // policy
    policy = SCHED_RR;
    // set for thread
    if( pthread_setschedparam( tid, policy, &param ) )
    {
        if( vm )
            vm->m_last_error = "could not set new scheduling parameters";
        return FALSE;
    }
    
    return TRUE;
}
#else
//-----------------------------------------------------------------------------
// name: set_priority()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::set_priority( t_CKINT priority, Chuck_VM * vm )
{
    // if priority is 0 then done
    if( !priority ) return TRUE;

    // set the priority class of the process
    // if( !SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS ) )
    //     return FALSE;

    // log
    EM_log( CK_LOG_INFO, "setting thread priority to: %ld...", priority );

    // set the priority the thread
    // if( !SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL ) )
    if( !SetThreadPriority( GetCurrentThread(), priority ) )
    {
        if( vm )
            vm->m_last_error = "could not set new scheduling parameters";
        return FALSE;
    }

    return TRUE;
}
#endif



        
//-----------------------------------------------------------------------------
// name: initialize()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::initialize( t_CKBOOL enable_audio, t_CKBOOL halt, t_CKUINT srate,
                               t_CKUINT buffer_size, t_CKUINT num_buffers,
                               t_CKUINT dac, t_CKUINT adc, t_CKUINT dac_chan,
                               t_CKUINT adc_chan, t_CKBOOL block, t_CKUINT adaptive )
{
    if( m_init )
    {
        m_last_error = "VM already initialized!";
        return FALSE;
    }

    // boost thread priority
    // if( priority != 0x7fffffff && !set_priority( priority, this ) )
    //    return FALSE;

    // log
    EM_log( CK_LOG_SYSTEM, "initializing virtual machine..." );
    EM_pushlog(); // push stack
    EM_log( CK_LOG_SYSTEM, "behavior: %s", halt ? "HALT" : "LOOP" );

    // lockdown
    Chuck_VM_Object::lock_all();

    // allocate bbq
    m_bbq = new BBQ;
    m_halt = halt;
    m_audio = enable_audio;
    m_block = block;
    
    // log
    EM_log( CK_LOG_SYSTEM, "allocating shreduler..." );
    // allocate shreduler
    m_shreduler = new Chuck_VM_Shreduler;
    m_shreduler->bbq = m_bbq;
    m_shreduler->rt_audio = enable_audio;
    m_shreduler->set_adaptive( adaptive > 0 ? adaptive : 0 );

    // log
    EM_log( CK_LOG_SYSTEM, "allocating messaging buffers..." );
    // allocate msg buffer
    m_msg_buffer = new CBufferSimple;
    m_msg_buffer->initialize( 1024, sizeof(Chuck_Msg *) );
    //m_msg_buffer->join(); // this should return 0
    m_reply_buffer = new CBufferSimple;
    m_reply_buffer->initialize( 1024, sizeof(Chuck_Msg *) );
    //m_reply_buffer->join(); // this should return 0 too
    m_event_buffer = new CBufferSimple;
    m_event_buffer->initialize( 1024, sizeof(Chuck_Event *) );
    //m_event_buffer->join(); // this should also return 0

    // log
    EM_log( CK_LOG_SYSTEM, "real-time audio: %s", enable_audio ? "YES" : "NO" );
    EM_log( CK_LOG_SYSTEM, "mode: %s", block ? "BLOCKING" : "CALLBACK" );
    EM_log( CK_LOG_SYSTEM, "sample rate: %ld", srate );
    EM_log( CK_LOG_SYSTEM, "buffer size: %ld", buffer_size );
    if( enable_audio )
    {
        EM_log( CK_LOG_SYSTEM, "num buffers: %ld", num_buffers );
        EM_log( CK_LOG_SYSTEM, "devices adc: %ld dac: %d (default 0)", adc, dac );
        EM_log( CK_LOG_SYSTEM, "adaptive block processing: %ld", adaptive > 1 ? adaptive : 0 ); 
    }
    EM_log( CK_LOG_SYSTEM, "channels in: %ld out: %ld", adc_chan, dac_chan );
    m_num_adc_channels = adc_chan;
    m_num_dac_channels = dac_chan;

    // at least set the sample rate and buffer size
    m_bbq->set_srate( srate );
    m_bbq->set_bufsize( buffer_size );
    m_bbq->set_numbufs( num_buffers );
    m_bbq->set_inouts( adc, dac );
    m_bbq->set_chans( adc_chan, dac_chan );

    // pop log
    EM_poplog();

    // TODO: clean up all the dynamic objects here on failure
    //       and in the shutdown function!

    return m_init = TRUE;
}




//-----------------------------------------------------------------------------
// name: initialize_synthesis()
// desc: requires type system
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::initialize_synthesis( )
{
    if( !m_init )
    {
        m_last_error = "VM initialize_synthesis() called on raw VM";
        return FALSE;
    }

    if( !g_t_dac || !g_t_adc )
    {
        m_last_error = "VM initialize_synthesis() called before type system initialized";
        return FALSE;
    }

    if( m_dac != NULL )
    {
        m_last_error = "VM synthesis already initialized";
        return FALSE;
    }

    // log
    EM_log( CK_LOG_SYSTEM, "initializing synthesis engine..." );
    // push indent
    EM_pushlog();

    // log
    EM_log( CK_LOG_SEVERE, "initializing 'dac'..." );
    // allocate dac and adc
    g_t_dac->ugen_info->num_outs = 
        g_t_dac->ugen_info->num_ins = m_num_dac_channels;
    m_dac = (Chuck_UGen *)instantiate_and_initialize_object( g_t_dac, NULL );
    object_ctor( m_dac, NULL, NULL ); // TODO: this can't be the place to do this
    stereo_ctor( m_dac, NULL, NULL ); // TODO: is the NULL shred a problem?
    multi_ctor( m_dac, NULL, NULL );  // TODO: remove and let type system do this
    m_dac->add_ref();
    // lock it
    m_dac->lock();

    // log
    EM_log( CK_LOG_SEVERE, "initializing 'adc'..." );
    g_t_adc->ugen_info->num_ins = 
        g_t_adc->ugen_info->num_outs = m_num_adc_channels;
    m_adc = (Chuck_UGen *)instantiate_and_initialize_object( g_t_adc, NULL );
    object_ctor( m_adc, NULL, NULL ); // TODO: this can't be the place to do this
    stereo_ctor( m_adc, NULL, NULL );
    multi_ctor( m_adc, NULL, NULL ); // TODO: remove and let type system do this
    m_adc->add_ref();
    // lock it
    m_adc->lock();

    // log
    EM_log( CK_LOG_SEVERE, "initializing 'blackhole'..." );
    m_bunghole = new Chuck_UGen;
    m_bunghole->add_ref();
    m_bunghole->lock();
    initialize_object( m_bunghole, &t_ugen );
    m_bunghole->tick = NULL;
    m_bunghole->alloc_v( m_shreduler->m_max_block_size );
    m_shreduler->m_dac = m_dac;
    m_shreduler->m_adc = m_adc;
    m_shreduler->m_bunghole = m_bunghole;
    m_shreduler->m_num_dac_channels = m_num_dac_channels;
    m_shreduler->m_num_adc_channels = m_num_adc_channels;

    // log
    EM_log( CK_LOG_SYSTEM, "initializing '%s' audio...", m_audio ? "real-time" : "fake-time" );
    // init bbq
    if( !m_bbq->initialize( m_num_dac_channels, m_num_adc_channels,
        Digitalio::m_sampling_rate, 16, 
        Digitalio::m_buffer_size, Digitalio::m_num_buffers,
        Digitalio::m_dac_n, Digitalio::m_adc_n,
        m_block, this, m_audio ) )
    {
        m_last_error = "cannot initialize audio device (try using --silent/-s)";
        // pop indent
        EM_poplog();
        return FALSE;
    }

    // pop indent
    EM_poplog();

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: compensate_bbq()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM::compensate_bbq()
{
    // set shreduler - the audio was initialized elsewhere
    m_shreduler->bbq = m_bbq;
}




//-----------------------------------------------------------------------------
// name: shutdown()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::shutdown()
{
    // make sure we are in the initialized state
    if( !m_init ) return FALSE;

    // log
    EM_log( CK_LOG_SYSTEM, "shutting down virtual machine..." );
    // push indent
    EM_pushlog();
    // unlockdown
    Chuck_VM_Object::unlock_all();

    // stop
    if( m_running )
    {
        this->stop();
        usleep( 50000 );
    }

    // shutdown audio
    if( m_audio )
    {
        // log
        EM_log( CK_LOG_SYSTEM, "shutting down real-time audio..." );

        m_bbq->digi_out()->cleanup();
        m_bbq->digi_in()->cleanup();
        m_bbq->shutdown();
        m_audio = FALSE;
    }
    // log
    EM_log( CK_LOG_SYSTEM, "freeing bbq subsystem..." );
    // clean up
    SAFE_DELETE( m_bbq );

    // log
    EM_log( CK_LOG_SYSTEM, "freeing shreduler..." );
    // free the shreduler
    SAFE_DELETE( m_shreduler );

    // log
    EM_log( CK_LOG_SYSTEM, "freeing msg/reply/event buffers..." );
    // free the msg buffer
    SAFE_DELETE( m_msg_buffer );
    // free the reply buffer
    SAFE_DELETE( m_reply_buffer );
    // free the event buffer
    SAFE_DELETE( m_event_buffer );

    // log
    EM_log( CK_LOG_SEVERE, "clearing shreds..." );
    // terminate shreds
    Chuck_VM_Shred * curr = m_shreds, * prev = NULL;
    while( curr )
    {
        prev = curr;
        curr = curr->next;
        // release shred
        prev->release();
    }
    m_shreds = NULL;
    m_num_shreds = 0;

    // log
    EM_pushlog();
    EM_log( CK_LOG_SEVERE, "freeing dumped shreds..." );
    // do it
    this->release_dump();
    EM_poplog();

    // log
    EM_log( CK_LOG_SYSTEM, "freeing special ugens..." );
    // go
    SAFE_RELEASE( m_dac );
    SAFE_RELEASE( m_adc );
    SAFE_RELEASE( m_bunghole );

    m_init = FALSE;

    // pop indent
    EM_poplog();

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: start_audio()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::start_audio( )
{
    // audio
    if( !m_audio_started && m_audio )
    {
        EM_log( CK_LOG_SEVERE, "starting real-time audio..." );
        m_bbq->digi_out()->start();
        m_bbq->digi_in()->start();
    }

    // set the flag to true to avoid entering this function
    m_audio_started = TRUE;

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: run()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::run( )
{
    // check if init
    if( m_dac == NULL )
    {
        m_last_error = "VM and/or synthesis not initialized...";
        return FALSE;
    }

    // check if already running
    if( m_running )
    {
        m_last_error = "virtual machine already running...";
        return FALSE;
    }

    m_running = TRUE;

    // log
    EM_log( CK_LOG_SYSTEM, "running virtual machine..." );
    // push indent
    EM_pushlog();

    // audio
    //if( m_audio )
    //{
        // log
        EM_log( CK_LOG_SEVERE, "initializing audio buffers..." );
        if( !m_bbq->digi_out()->initialize( ) )
        {
            m_last_error = "cannot open audio output (option: use --silent/-s)";
            return FALSE;
        }

        m_bbq->digi_in()->initialize( );
    //}

    // log
    EM_log( CK_LOG_SEVERE, "virtual machine running..." );
    // pop indent
    EM_poplog();

    // run
    if( m_block ) this->run( -1 );
    else
    {
        // compute shreds before first sample
        if( !compute() )
        {
            // done
            m_running = FALSE;
            // log
            EM_log( CK_LOG_SYSTEM, "virtual machine stopped..." );
        }
        else
        {
            // start audio
            if( !m_audio_started ) start_audio();

            // wait
            while( m_running )
            { usleep( 50000 ); }
        }
    }

    return TRUE;
}

/*should we comment out what we just did?
i can't think of why it might be affecting this part of the vm
you never know
true*/




//-----------------------------------------------------------------------------
// name: compute()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::compute()
{
    Chuck_VM_Shred *& shred = m_shreduler->m_current_shred;
    Chuck_Msg * msg = NULL;
    Chuck_Event * event = NULL;
    t_CKBOOL iterate = TRUE;

    // iteration until no more shreds/events/messages
    while( iterate )
    {
        // get the shreds queued for 'now'
        while(( shred = m_shreduler->get() ))
        {
            // set the current time of the shred
            shred->now = shred->wake_time;

            // track shred activation
            CK_TRACK( Chuck_Stats::instance()->activate_shred( shred ) );

            // run the shred
            if( !shred->run( this ) )
            {
                // track shred deactivation
                CK_TRACK( Chuck_Stats::instance()->deactivate_shred( shred ) );

                this->free( shred, TRUE );
                shred = NULL;
                if( !m_num_shreds && m_halt ) return FALSE;
            }

            // track shred deactivation
            CK_TRACK( if( shred ) Chuck_Stats::instance()->deactivate_shred( shred ) );

            // zero out
            shred = NULL;
        }

        // set to false for now
        iterate = FALSE;

        // broadcast queued events
        while( m_event_buffer->get( &event, 1 ) )
        { event->broadcast(); iterate = TRUE; }

        // process messages
        while( m_msg_buffer->get( &msg, 1 ) )
        { process_msg( msg ); iterate = TRUE; }

        // clear dumped shreds
        if( m_num_dumped_shreds > 0 )
            release_dump();
    }
    
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: run()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::run( t_CKINT num_samps )
{
    // loop it
    while( num_samps )
    {
        // compute shreds
        if( !compute() ) goto vm_stop;

        // start audio
        if( !m_audio_started ) start_audio();

        // advance the shreduler
        if( !m_shreduler->m_adaptive )
        {
            m_shreduler->advance();
            if( num_samps > 0 ) num_samps--;
        }
        else m_shreduler->advance_v( num_samps );
    }

    return FALSE;

// vm stop here
vm_stop:
    m_running = FALSE;

    // log
    EM_log( CK_LOG_SYSTEM, "virtual machine stopped..." );

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: pause()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::pause( )
{
    m_running = FALSE;

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: stop()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::stop( )
{
    // log
    EM_log( CK_LOG_SEVERE, "requesting STOP virtual machine..." );

    m_running = FALSE;
    Digitalio::m_end = TRUE;

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: gc
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM::gc( t_CKUINT amount )
{
}




//-----------------------------------------------------------------------------
// name: gc
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM::gc( )
{
}




//-----------------------------------------------------------------------------
// name: queue_msg()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::queue_msg( Chuck_Msg * msg, int count )
{
    assert( count == 1 );
    m_msg_buffer->put( &msg, count );
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: queue_event()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::queue_event( Chuck_Event * event, int count )
{
    assert( count == 1 );
    m_event_buffer->put( &event, count );
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: get_reply()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_Msg * Chuck_VM::get_reply( )
{
    Chuck_Msg * msg = NULL;
    m_reply_buffer->get( &msg, 1 );
    return msg;
}




//-----------------------------------------------------------------------------
// name: process_msg()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_VM::process_msg( Chuck_Msg * msg )
{
    t_CKUINT retval = 0xfffffff0;

    if( msg->type == MSG_REPLACE )
    {
        Chuck_VM_Shred * out = m_shreduler->lookup( msg->param );
        if( !out )
        {
            EM_error3( "[chuck](VM): error replacing shred: no shred with id %i...",
                       msg->param );
            retval = 0;
            goto done;
        }

        Chuck_VM_Shred * shred = msg->shred;
        if( !shred )
        {
            shred = new Chuck_VM_Shred;
            shred->initialize( msg->code );
            shred->name = msg->code->name;
            shred->base_ref = shred->mem;
            shred->add_ref();
        }
        // set the current time
        shred->start = m_shreduler->now_system;
        // set the id
        shred->xid = msg->param;
        // set the now
        shred->now = shred->wake_time = m_shreduler->now_system;
        // set the vm
        shred->vm_ref = this;
        // set args
        if( msg->args ) shred->args = *(msg->args);
        // add it to the parent
        if( shred->parent )
            shred->parent->children[shred->xid] = shred;

        // replace
        if( m_shreduler->remove( out ) && m_shreduler->shredule( shred ) )
        {
            EM_error3( "[chuck](VM): replacing shred %i (%s) with %i (%s)...",
                       out->xid, mini(out->name.c_str()), shred->xid, mini(shred->name.c_str()) );
            this->free( out, TRUE, FALSE );
            retval = shred->xid;
        
            // tracking new shred
            CK_TRACK( Chuck_Stats::instance()->add_shred( shred ) );

            goto done;
        }
        else
        {
            EM_error3( "[chuck](VM): shreduler ERROR replacing shred %i...",
                       out->xid );
            shred->release();
            retval = 0;
            goto done;
        }
    }
    else if( msg->type == MSG_REMOVE )
    {
        if( msg->param == 0xffffffff )
        {
            if( !this->m_num_shreds)
            {
                EM_error3( "[chuck](VM): no shreds to remove..." );
                retval = 0;
                goto done;
            }

            t_CKINT xid = m_shred_id;
            Chuck_VM_Shred * shred = NULL;
            while( xid >= 0 && m_shreduler->remove( shred = m_shreduler->lookup( xid ) ) == 0 )
                xid--;
            if( xid >= 0 )
            {
                EM_error3( "[chuck](VM): removing recent shred: %i (%s)...", 
                           xid, mini(shred->name.c_str()) );
                this->free( shred, TRUE );
                retval = xid;
            }
            else
            {
                EM_error3( "[chuck](VM): no shreds removed..." );
                retval = 0;
                goto done;
            }
        }
        else
        {
            Chuck_VM_Shred * shred = m_shreduler->lookup( msg->param );
            if( !shred )
            {
                EM_error3( "[chuck](VM): cannot remove: no shred with id %i...",
                           msg->param );
                retval = 0;
                goto done;
            }
            if( shred != m_shreduler->m_current_shred && !m_shreduler->remove( shred ) )  // was lookup
            {
                EM_error3( "[chuck](VM): shreduler: cannot remove shred %i...",
                           msg->param );
                retval = 0;
                goto done;
            }
            EM_error3( "[chuck](VM): removing shred: %i (%s)...",
                       msg->param, mini(shred->name.c_str()) );
            this->free( shred, TRUE );
            retval = msg->param;
        }
    }
    else if( msg->type == MSG_REMOVEALL )
    {
        t_CKUINT xid = m_shred_id;
        EM_error3( "[chuck](VM): removing all (%i) shreds...", m_num_shreds );
        Chuck_VM_Shred * shred = NULL;

        while( m_num_shreds && xid > 0 )
        {
            if( m_shreduler->remove( shred = m_shreduler->lookup( xid ) ) )
                this->free( shred, TRUE );
            xid--;
        }
        
        m_shred_id = 0;
        m_num_shreds = 0;
    }
    else if( msg->type == MSG_ADD )
    {
        t_CKUINT xid = 0;
        Chuck_VM_Shred * shred = NULL;
        if( msg->shred ) shred = this->spork( msg->shred );
        else shred = this->spork( msg->code, NULL );
        xid = shred->xid;
        if( msg->args ) shred->args = *(msg->args);

        const char * s = ( msg->shred ? msg->shred->name.c_str() : msg->code->name.c_str() );
        EM_error3( "[chuck](VM): sporking incoming shred: %i (%s)...", xid, mini(s) );
        retval = xid;
        goto done;
    }
    else if( msg->type == MSG_KILL )
    {
        EM_error3( "[chuck](VM): KILL received...." );
        // close file handles and clean up
        all_detach();
        // TODO: free more memory?

        // log
        EM_log( CK_LOG_INFO, "(VM): exiting..." );
        // come again
        exit( 1 );
    }
    else if( msg->type == MSG_STATUS )
    {
        // fill in structure
        if( msg->user && msg->reply )
        {
            // cast
            Chuck_VM_Status * status = (Chuck_VM_Status *)msg->user;
            // get it
            m_shreduler->status( status );
        }
        else
        {
            m_shreduler->status();
        }
    }
    else if( msg->type == MSG_TIME )
    {
        float srate = (float)Digitalio::sampling_rate();
        fprintf( stderr, "[chuck](VM): the values of now:\n" );
        fprintf( stderr, "  now = %.6f (samp)\n", m_shreduler->now_system );
        fprintf( stderr, "      = %.6f (second)\n", m_shreduler->now_system / srate );
        fprintf( stderr, "      = %.6f (minute)\n", m_shreduler->now_system / srate / 60.0f );
        fprintf( stderr, "      = %.6f (hour)\n", m_shreduler->now_system / srate / 60.0f / 60.0f );
        fprintf( stderr, "      = %.6f (day)\n", m_shreduler->now_system / srate / 60.0f / 60.0f / 24.0f );
        fprintf( stderr, "      = %.6f (week)\n", m_shreduler->now_system / srate / 60.0f / 60.0f / 24.0f / 7.0f );
    }
    else if( msg->type == MSG_RESET_ID )
    {
        t_CKUINT n = m_shreduler->highest();
        m_shred_id = n;
        fprintf( stderr, "[chuck](VM): reseting shred id to %d...\n", m_shred_id + 1 );
    }

done:

    if( msg->reply )
    {
        msg->replyA = retval;
        m_reply_buffer->put( &msg, 1 );
    }
    else
        SAFE_DELETE(msg);

    return retval;
}




//-----------------------------------------------------------------------------
// name: next_id()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_VM::next_id( )
{
    return ++m_shred_id;
}




//-----------------------------------------------------------------------------
// name: shreduler()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shreduler * Chuck_VM::shreduler( ) const
{
    return m_shreduler;
}




//-----------------------------------------------------------------------------
// name: bbq()
// desc: ...
//-----------------------------------------------------------------------------
BBQ * Chuck_VM::bbq( ) const
{
    return m_bbq;
}




//-----------------------------------------------------------------------------
// name: srate()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_VM::srate() const
{
    return (t_CKUINT)Digitalio::sampling_rate();
}




//-----------------------------------------------------------------------------
// name: fork()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shred * Chuck_VM::fork( Chuck_VM_Code * code )
{
    return NULL;
}




//-----------------------------------------------------------------------------
// name: spork()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shred * Chuck_VM::spork( Chuck_VM_Code * code, Chuck_VM_Shred * parent )
{
    // allocate a new shred
    Chuck_VM_Shred * shred = new Chuck_VM_Shred;
    // initialize the shred (default stack size)
    shred->initialize( code );
    // set the name
    shred->name = code->name;
    // set the parent
    shred->parent = parent;
    // set the base ref for global
    if( parent ) shred->base_ref = shred->parent->base_ref;
    else shred->base_ref = shred->mem;
    // spork it
    this->spork( shred );

    // track new shred
    CK_TRACK( Chuck_Stats::instance()->add_shred( shred ) );

    return shred;
}




//-----------------------------------------------------------------------------
// name: spork()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shred * Chuck_VM::spork( Chuck_VM_Shred * shred )
{
    // set the current time
    shred->start = m_shreduler->now_system;
    // set the now
    shred->now = shred->wake_time = m_shreduler->now_system;
    // set the id
    shred->xid = next_id();
    // set the vm
    shred->vm_ref = this;
    // add ref
    shred->add_ref();
    // add it to the parent
    if( shred->parent )
        shred->parent->children[shred->xid] = shred;
    // shredule it
    m_shreduler->shredule( shred );
    // count
    m_num_shreds++;

    return shred;
}




//-----------------------------------------------------------------------------
// name: free()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::free( Chuck_VM_Shred * shred, t_CKBOOL cascade, t_CKBOOL dec )
{
    assert( cascade );
    
    // log
    EM_log( CK_LOG_FINER, "freeing shred (id==%d | ptr==%p)", shred->xid,
            (t_CKUINT)shred );

    // abort on the double free
    // TODO: can a shred be dumped, then resporked? from code?
    if( shred->is_dumped ) return FALSE;

    // mark this done
    shred->is_done = TRUE;

    // free the children
    t_CKINT size = shred->children.size();
    if( size )
    {
        vector<Chuck_VM_Shred *> list; list.resize( size );
        map<t_CKUINT, Chuck_VM_Shred *>::iterator iter; t_CKINT i = 0;
        for( iter = shred->children.begin(); iter != shred->children.end(); iter++ )
            list[i++] = (*iter).second;
        for( i = 0; i < size; i++ )
            this->free( list[i], cascade );
    }

    // make sure it's done
    assert( shred->children.size() == 0 );

    // tell parent
    if( shred->parent )
        shred->parent->children.erase( shred->xid );

    // track remove shred
    CK_TRACK( Chuck_Stats::instance()->remove_shred( shred ) );

    // free!
    m_shreduler->remove( shred );
    // TODO: remove shred from event, with synchronization (still necessary with dump?)
    // if( shred->event ) shred->event->remove( shred );
    // OLD: shred->release();
    this->dump( shred );
    shred = NULL;
    if( dec ) m_num_shreds--;
    if( !m_num_shreds ) m_shred_id = 0;
    
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: abort_current_shred()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM::abort_current_shred( )
{
    // for threading
    Chuck_VM_Shred * shred = m_shreduler->m_current_shred;
    
    // if there
    if( shred )
    {
        // log
        EM_log( CK_LOG_SEVERE, "trying to abort current shred (id: %d)", shred->xid );
        // flag it
        shred->is_abort = TRUE;
    }
    else
    {
        // log
        EM_log( CK_LOG_SEVERE, "cannot abort shred: nothing currently running!" );
    }

    return shred != NULL;
}




//-----------------------------------------------------------------------------
// name: dump()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM::dump( Chuck_VM_Shred * shred )
{
    // log
    EM_log( CK_LOG_FINER, "dumping shred (id==%d | ptr==%p)", shred->xid,
            (t_CKUINT)shred );
    // add
    m_shred_dump.push_back( shred );
    // stop
    shred->is_running = FALSE;
    shred->is_done = TRUE;
    shred->is_dumped = TRUE;
    // TODO: cool?
    shred->xid = 0;
    // inc
    m_num_dumped_shreds++;
}




//-----------------------------------------------------------------------------
// name: release_dump()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM::release_dump( )
{
    // log
    EM_log( CK_LOG_FINER, "releasing dumped shreds..." );

    // iterate through dump
    for( t_CKUINT i = 0; i < m_shred_dump.size(); i++ )
        SAFE_RELEASE( m_shred_dump[i] );

    // clear the dump
    m_shred_dump.clear();
    // reset
    m_num_dumped_shreds = 0;
}




//-----------------------------------------------------------------------------
// name: Chuck_VM_Stack()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Stack::Chuck_VM_Stack()
{
    stack = sp = sp_max = NULL;
    prev = next = NULL;
    m_is_init = FALSE;
}




//-----------------------------------------------------------------------------
// name: ~Chuck_VM_Stack()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Stack::~Chuck_VM_Stack()
{
    this->shutdown();
}




//-----------------------------------------------------------------------------
// name: Chuck_VM_Code()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Code::Chuck_VM_Code()
{
    instr = NULL;
    num_instr = 0;
    stack_depth = 0;
    need_this = FALSE;
    native_func = 0;
    native_func_type = NATIVE_UNKNOWN;
}




//-----------------------------------------------------------------------------
// name: ~Chuck_VM_Code()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Code::~Chuck_VM_Code()
{
    // free instructions
    if( instr )
    {
        // loop over array
        for( t_CKUINT i = 0; i < num_instr; i++ )
            delete instr[i];

        // free the array
        SAFE_DELETE_ARRAY( instr );
    }

    num_instr = 0;
}




// offset in bytes at the beginning of a stack for initializing data
#define VM_STACK_OFFSET  16
// 1/factor of stack is left blank, to give room to detect overflow
#define VM_STACK_PADDING_FACTOR 16
//-----------------------------------------------------------------------------
// name: initialize()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Stack::initialize( t_CKUINT size )
{
    if( m_is_init )
        return FALSE;

    // make room for header
    size += VM_STACK_OFFSET;
    // allocate stack
    stack = new t_CKBYTE[size];
    if( !stack ) goto out_of_memory;

    // zero
    memset( stack, 0, size );

    // advance stack after the header
    stack += VM_STACK_OFFSET;
    // set the sp
    sp = stack;
    // upper limit (padding factor)
    sp_max = sp + size - (size / VM_STACK_PADDING_FACTOR);

    // set flag and return
    return m_is_init = TRUE;

out_of_memory:

    // we have a problem
    fprintf( stderr, 
        "[chuck](VM): OutOfMemory: while allocating stack '%s'\n" );

    // return FALSE
    return FALSE;
}




//-----------------------------------------------------------------------------
// name: shutdown()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Stack::shutdown()
{
    if( !m_is_init )
        return FALSE;

    // free the stack
    stack -= VM_STACK_OFFSET;
    SAFE_DELETE_ARRAY( stack );
    sp = sp_max = NULL;

    // set the flag to false
    m_is_init = FALSE;

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: Chuck_VM_Shred()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shred::Chuck_VM_Shred()
{
    mem = new Chuck_VM_Stack;
    reg = new Chuck_VM_Stack;
    code = NULL;
    next = prev = NULL;
    instr = NULL;
    parent = NULL;
    // obj_array = NULL;
    // obj_array_size = 0;
    base_ref = NULL;
    vm_ref = NULL;
    event = NULL;
    xid = 0;

    // set
    CK_TRACK( stat = NULL );
}




//-----------------------------------------------------------------------------
// name: ~Chuck_VM_Shred()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shred::~Chuck_VM_Shred()
{
    this->shutdown();
}




//-----------------------------------------------------------------------------
// name: initialize()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shred::initialize( Chuck_VM_Code * c,
                                     t_CKUINT mem_stack_size, 
                                     t_CKUINT reg_stack_size )
{
    // allocate mem and reg
    if( !mem->initialize( mem_stack_size ) ) return FALSE;
    if( !reg->initialize( reg_stack_size ) ) return FALSE;

    // program counter
    pc = 0;
    next_pc = 1;
    // code pointer
    code_orig = code = c;
    // add reference
    code_orig->add_ref();
    // shred in dump (all done)
    is_dumped = FALSE;
    // shred done
    is_done = FALSE;
    // shred running
    is_running = FALSE;
    // shred abort
    is_abort = FALSE;
    // set the instr
    instr = c->instr;
    // zero out the id
    xid = 0;

    // initialize
    initialize_object( this, &t_shred );

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: shutdown()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shred::shutdown()
{
    // get iterator to our map
    map<Chuck_UGen *, Chuck_UGen *>::iterator iter = m_ugen_map.begin();
    while( iter != m_ugen_map.end() )
    {
        (*iter).first->disconnect( TRUE );
        iter++;
    }
    m_ugen_map.clear();

    SAFE_DELETE( mem );
    SAFE_DELETE( reg );
    base_ref = NULL;
    
    // delete temp pointer space
    // SAFE_DELETE_ARRAY( obj_array );
    // obj_array_size = 0;

    // TODO: is this right?
    code_orig->release();
    code_orig = code = NULL;
    // what to do with next and prev?

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: add()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shred::add( Chuck_UGen * ugen )
{
    if( m_ugen_map[ugen] )
        return FALSE;

    m_ugen_map[ugen] = ugen;
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: remove()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shred::remove( Chuck_UGen * ugen )
{
    if( !m_ugen_map[ugen] )
        return FALSE;
    
    // remove it
    m_ugen_map.erase( ugen );
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: run()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shred::run( Chuck_VM * vm )
{
    // get the code
    instr = code->instr;
    is_running = TRUE;
    t_CKBOOL * vm_running = &vm->m_running;

    // go!
    while( is_running && *vm_running && !is_abort )
    {
        // execute the instruction
        instr[pc]->execute( vm, this );

        // set to next_pc;
        pc = next_pc;
        next_pc++;

        // track number of cycles
        CK_TRACK( this->stat->cycles++ );
    }
    
    // check abort
    if( is_abort )
    {
        // log
        EM_log( CK_LOG_SYSTEM, "aborting shred (id: %d)", this->xid );
        // done
        is_done = TRUE;
    }

    // is the shred finished
    return !is_done;
}




//-----------------------------------------------------------------------------
// name: Chuck_VM_Shreduler()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shreduler::Chuck_VM_Shreduler()
{
    now_system = 0;
    rt_audio = FALSE;
    bbq = NULL;
    shred_list = NULL;
    m_current_shred = NULL;
    m_dac = NULL;
    m_adc = NULL;
    m_bunghole = NULL;
    m_num_dac_channels = 0;
    m_num_adc_channels = 0;
    
    set_adaptive( 0 );
}




//-----------------------------------------------------------------------------
// name: ~Chuck_VM_Shreduler()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shreduler::~Chuck_VM_Shreduler()
{
    this->shutdown();
}




//-----------------------------------------------------------------------------
// name: initialize()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::initialize()
{
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: shutdown()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::shutdown()
{
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: set_adaptive()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM_Shreduler::set_adaptive( t_CKUINT max_block_size )
{
    m_max_block_size = max_block_size > 1 ? max_block_size : 0;
    m_adaptive = m_max_block_size > 1;
    m_samps_until_next = -1;
}




//-----------------------------------------------------------------------------
// name: add_blocked()
// desc: add shred to the shreduler's blocked list
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::add_blocked( Chuck_VM_Shred * shred )
{
    // add shred to map, using pointer
    blocked[shred] = shred;
    
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: remove_blocked()
// desc: remove shred from the shreduler's blocked list
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::remove_blocked( Chuck_VM_Shred * shred )
{
    // remove from hash
    std::map<Chuck_VM_Shred *, Chuck_VM_Shred *>::iterator iter;
    iter = blocked.find( shred );
    blocked.erase( iter );

    // remove from event
    if( shred->event != NULL ) shred->event->remove( shred );
    
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: shredule()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::shredule( Chuck_VM_Shred * shred )
{
    return this->shredule( shred, now_system );
}




//-----------------------------------------------------------------------------
// name: shredule()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::shredule( Chuck_VM_Shred * shred,
                                       t_CKTIME wake_time )
{
    // sanity check
    if( shred->prev || shred->next )
    {
        // something is really wrong here - no shred can be 
        // shreduled more than once
        EM_error3( "[chuck](VM): internal sanity check failed in shredule()" );
        EM_error3( "[chuck](VM): (shred shreduled while shreduled)" );

        return FALSE;
    }

    // sanity check
    if( wake_time < (this->now_system - .5) )
    {
        // trying to enqueue on a time that is less than now
        EM_error3( "[chuck](VM): internal sanity check failed in shredule()" );
        EM_error3( "[chuck](VM): (wake time is past) - %f : %f", wake_time, this->now_system );

        return FALSE;
    }

    shred->wake_time = wake_time;

    // list empty
    if( !shred_list )
        shred_list = shred;
    else
    {
        // pointers to the shred queue
        Chuck_VM_Shred * curr = shred_list;
        Chuck_VM_Shred * prev = NULL;

        while( curr )
        {
            // found the place to insert
            if( curr->wake_time > wake_time )
                break;

            prev = curr;
            curr = curr->next;
        }

        if( !prev )
        {
            shred->next = shred_list;
            if( shred_list ) shred_list->prev = shred;
            shred_list = shred;
        }
        else
        {
            // insert the shred in sorted order
            shred->next = prev->next;
            shred->prev = prev;
            if( prev->next ) prev->next->prev = shred;
            prev->next = shred;
        }
    }

    t_CKTIME diff = shred_list->wake_time - this->now_system;
    if( diff < 0 ) diff = 0;
    // if( diff < m_samps_until_next )
    m_samps_until_next = diff;
    
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: advance_v()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM_Shreduler::advance_v( t_CKINT & numLeft )
{
    t_CKINT i, j, numFrames;
    SAMPLE frame[128], gain[128], sum;
    BBQ * audio = this->bbq;
    
    // compute number of frames to compute; update
    numFrames = ck_min( m_max_block_size, numLeft );
    if( this->m_samps_until_next >= 0 )
    {
        numFrames = (t_CKINT)(ck_min( numFrames, this->m_samps_until_next ));
        if( numFrames == 0 ) numFrames = 1;
        this->m_samps_until_next -= numFrames;
    }
    numLeft -= numFrames;

    // advance system 'now'
    this->now_system += numFrames;

    // tick in
    if( rt_audio )
    {
        for( j = 0; j < m_num_adc_channels; j++ )
        {
            // update channel
            m_adc->m_multi_chan[j]->m_time = this->now_system;
            // cache gain
            gain[j] = m_adc->m_multi_chan[j]->m_gain;
        }
        
        // adaptive block
        for( i = 0; i < numFrames; i++ )
        {
            // get input
            audio->digi_in()->tick_in( frame, m_num_adc_channels );
            // clear
            sum = 0.0f;
            // loop over channels
            for( j = 0; j < m_num_adc_channels; j++ )
            {
                m_adc->m_multi_chan[j]->m_current_v[i] = frame[j] * gain[j] * m_adc->m_gain;
                sum += m_adc->m_multi_chan[j]->m_current_v[i];
            }
            m_adc->m_current_v[i] = sum / m_num_adc_channels;
        }
        
        for( j = 0; j < m_num_adc_channels; j++ )
        {
            // update last
            m_adc->m_multi_chan[j]->m_last = m_adc->m_multi_chan[j]->m_current_v[numFrames-1];
        }
        // update last
        m_adc->m_last = m_adc->m_current_v[numFrames-1];
        // update time
        m_adc->m_time = this->now_system;
    }

    // dac
    m_dac->system_tick_v( this->now_system, numFrames );

    // suck samples
    m_bunghole->system_tick_v( this->now_system, numFrames );

    // adaptive block
    for( i = 0; i < numFrames; i++ )
    {
        for( j = 0; j < m_num_dac_channels; j++ )
            frame[j] = m_dac->m_multi_chan[j]->m_current_v[i];
        
        // tick
        audio->digi_out()->tick_out( frame, m_num_dac_channels );
    }
}




//-----------------------------------------------------------------------------
// name: advance2()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM_Shreduler::advance2( )
{
    // advance system 'now'
    this->now_system += 1;

    // tick the dac
    SAMPLE l, r;
    BBQ * audio = this->bbq;

    // tick in
    if( rt_audio )
    {
        audio->digi_in()->tick_in( &l, &r );
        m_adc->m_multi_chan[0]->m_current = l * m_adc->m_multi_chan[0]->m_gain;
        m_adc->m_multi_chan[1]->m_current = r * m_adc->m_multi_chan[1]->m_gain;
        m_adc->m_current = .5f * ( l + r );
        // time it
        m_adc->m_multi_chan[0]->m_time = this->now_system;
        m_adc->m_multi_chan[1]->m_time = this->now_system;
        m_adc->m_time = this->now_system;
    }

    // dac
    m_dac->system_tick( this->now_system );
    l = m_dac->m_multi_chan[0]->m_current;
    r = m_dac->m_multi_chan[1]->m_current;
    // remove: 1.2.1.2
    // l *= .5f; r *= .5f;

    // suck samples
    m_bunghole->system_tick( this->now_system );

    // tick
    audio->digi_out()->tick_out( l, r );
}




//-----------------------------------------------------------------------------
// name: advance()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM_Shreduler::advance( )
{
    // advance system 'now'
    this->now_system += 1;

    // tick the dac
    SAMPLE frame[128];
    SAMPLE sum = 0.0f;
    BBQ * audio = this->bbq;
    t_CKUINT i;

    // tick in
    if( rt_audio )
    {
        audio->digi_in()->tick_in( frame, m_num_adc_channels );
        
        // loop over channels
        for( i = 0; i < m_num_adc_channels; i++ )
        {
            m_adc->m_multi_chan[i]->m_current = frame[i] * m_adc->m_multi_chan[i]->m_gain * m_adc->m_gain;
            m_adc->m_multi_chan[i]->m_last = m_adc->m_multi_chan[i]->m_current;
            m_adc->m_multi_chan[i]->m_time = this->now_system;
            sum += m_adc->m_multi_chan[i]->m_current;
        }
        m_adc->m_last = m_adc->m_current = sum / m_num_adc_channels;
        m_adc->m_time = this->now_system;
    }

    // dac
    m_dac->system_tick( this->now_system );
    for( i = 0; i < m_num_dac_channels; i++ )
        frame[i] = m_dac->m_multi_chan[i]->m_current; // * .5f;

    // suck samples
    m_bunghole->system_tick( this->now_system );

    // tick
    audio->digi_out()->tick_out( frame, m_num_dac_channels );
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shred * Chuck_VM_Shreduler::get( )
{
    Chuck_VM_Shred * shred = shred_list;

    // list empty
    if( !shred )
    {
        m_samps_until_next = -1;
        return NULL;
    }

    // TODO: should this be <=?
    if( shred->wake_time <= ( this->now_system + .5 ) )
    {
        // if( shred->wake_time < this->now_system )
        //    assert( false );

        shred_list = shred->next;
        shred->next = NULL;
        shred->prev = NULL;
        
        if( shred_list )
        {
            shred_list->prev = NULL;
            m_samps_until_next = shred_list->wake_time - this->now_system;
            if( m_samps_until_next < 0 ) m_samps_until_next = 0;
        }

        return shred;
    }

    return NULL;
}




//-----------------------------------------------------------------------------
// name: highest()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_VM_Shreduler::highest( )
{
    Chuck_VM_Shred * shred = shred_list;
    t_CKUINT n = 0;

    while( shred )
    {
        if( shred->xid > n ) n = shred->xid;
        shred = shred->next;
    }

    std::map<Chuck_VM_Shred *, Chuck_VM_Shred *>::iterator iter;    
    for( iter = blocked.begin(); iter != blocked.end(); iter++ )
    {
        shred = (*iter).second;
        if( shred->xid > n ) n = shred->xid;
    }

    return n;
}




//-----------------------------------------------------------------------------
// name: replace()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::replace( Chuck_VM_Shred * out, Chuck_VM_Shred * in )
{
    assert( FALSE );

    // sanity check
    if( !out || !in )
        return FALSE;

    if( !out->prev )
        shred_list = in;
    else
        out->prev->next = in;
    
    if( out->next )
        out->next->prev = in;
    
    in->next = out->next;
    in->prev = out->prev;
    
    out->next = out->prev = NULL;

    in->wake_time = out->wake_time;
    in->start = in->wake_time;
    
    return TRUE;
}




//-----------------------------------------------------------------------------
// name: remove()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_VM_Shreduler::remove( Chuck_VM_Shred * out )
{
    if( !out ) return FALSE;

    // if blocked
    if( out->event != NULL )
    {
        return remove_blocked( out );
    }

    // sanity check
    if( !out->prev && !out->next && out != shred_list )
        return FALSE;
    
    if( !out->prev )
        shred_list = out->next;
    else
        out->prev->next = out->next;
    
    if( out->next )
        out->next->prev = out->prev;
    
    out->next = out->prev = NULL;

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Shred * Chuck_VM_Shreduler::lookup( t_CKUINT xid )
{
    Chuck_VM_Shred * shred = shred_list;

    // current shred?
    if( m_current_shred != NULL && m_current_shred->xid == xid )
        return m_current_shred;

    // look for in shreduled list
    while( shred )
    {
        if( shred->xid == xid )
            return shred;

        shred = shred->next;
    }

    // blocked?
    std::map<Chuck_VM_Shred *, Chuck_VM_Shred *>::iterator iter;
    for( iter = blocked.begin(); iter != blocked.end(); iter++ )
    {
        shred = (*iter).second;
        if( shred->xid == xid )
            return shred;
    }
    
    return NULL;
}




//-----------------------------------------------------------------------------
// name: SortByID()
// desc: ...
//-----------------------------------------------------------------------------
struct SortByID
{
    bool operator() ( const Chuck_VM_Shred * lhs, const Chuck_VM_Shred * rhs )
    {    return lhs->xid < rhs->xid; }
};




//-----------------------------------------------------------------------------
// name: status()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM_Shreduler::status( Chuck_VM_Status * status )
{
    Chuck_VM_Shred * shred = shred_list;
    Chuck_VM_Shred * temp = NULL;

    t_CKUINT srate = Digitalio::sampling_rate();
    t_CKUINT s = (t_CKUINT)now_system;
    t_CKUINT h = s/(srate*3600);
    s = s - (h*(srate*3600));
    t_CKUINT m = s / (srate*60);
    s = s - (m*(srate*60));
    t_CKUINT sec = s / srate;
    s = s - (sec*(srate));
    // float millisecond = s / (float)(srate) * 1000.0f;
    
    status->srate = srate;
    status->now_system = now_system;
    status->t_second = sec;
    status->t_minute = m;
    status->t_hour = h;
    
    // get list of shreds
    vector<Chuck_VM_Shred *> list;
    while( shred )
    {
        list.push_back( shred );
        shred = shred->next;
    }

    // get blocked
    std::map<Chuck_VM_Shred *, Chuck_VM_Shred *>::iterator iter;    
    for( iter = blocked.begin(); iter != blocked.end(); iter++ )
    {
        shred = (*iter).second;
        list.push_back( shred );
    }

    // get current shred
    if( temp = m_current_shred )
        list.push_back( temp );

    // sort the list
    SortByID byid;
    std::sort( list.begin(), list.end(), byid );

    // print status
    status->clear();
    for( t_CKUINT i = 0; i < list.size(); i++ )
    {
        shred = list[i];
        status->list.push_back( new Chuck_VM_Shred_Status(
            shred->xid, shred->name, shred->start, shred->event != NULL ) );
    }    
}




//-----------------------------------------------------------------------------
// name: status()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM_Shreduler::status( )
{
    Chuck_VM_Shred_Status * shred = NULL;

    this->status( &m_status );
    t_CKUINT h = m_status.t_hour;
    t_CKUINT m = m_status.t_minute;
    t_CKUINT sec = m_status.t_second;
    fprintf( stdout, "[chuck](VM): status (now == %ldh%ldm%lds, %.1f samps) ...\n",
             h, m, sec, m_status.now_system );

    // print status
    for( t_CKUINT i = 0; i < m_status.list.size(); i++ )
    {
        shred = m_status.list[i];
        fprintf( stdout, 
            "    [shred id]: %ld  [source]: %s  [spork time]: %.2fs ago%s\n",
            shred->xid, mini( shred->name.c_str() ),
            (m_status.now_system - shred->start) / m_status.srate,
            shred->has_event ? " (blocked)" : "" );
    }
}




//-----------------------------------------------------------------------------
// name: Chuck_VM_Status()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Status::Chuck_VM_Status()
{
    srate = 0;
    now_system = 0;
    t_second = t_minute = t_hour = 0;
}




//-----------------------------------------------------------------------------
// name: ~Chuck_VM_Status()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_VM_Status::~Chuck_VM_Status()
{
    this->clear();
}




//-----------------------------------------------------------------------------
// name: clear()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_VM_Status::clear()
{
    for( t_CKUINT i = 0; i < list.size(); i++ )
    {
        SAFE_DELETE( list[i] );
    }
    
    list.clear();
}
