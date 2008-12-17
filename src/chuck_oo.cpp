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
// file: chuck_oo.cpp
// desc: ...
//
// author: Ge Wang (gewang@cs.princeton.edu)
//         Perry R. Cook (prc@cs.princeton.edu)
//         Ananya Misra (amisra@cs.princeton.edu)
// date: Autumn 2004
//-----------------------------------------------------------------------------
#include "chuck_oo.h"
#include "chuck_type.h"
#include "chuck_vm.h"
#include "chuck_instr.h"
#include "chuck_errmsg.h"

#include <sstream>
#include <iomanip>
#include <typeinfo>
using namespace std;


// initialize
t_CKBOOL Chuck_VM_Object::our_locks_in_effect = TRUE;
const t_CKINT Chuck_IO::READ = 0x1;
const t_CKINT Chuck_IO::WRITE = 0x2;
const t_CKINT Chuck_IO::APPEND = 0x4;
const t_CKINT Chuck_IO::TRUNCATE = 0x8;
const t_CKINT Chuck_IO::ASCII = 0x10;
const t_CKINT Chuck_IO::BINARY = 0x20;




//-----------------------------------------------------------------------------
// name: init_ref()
// desc: initialize vm object
//-----------------------------------------------------------------------------
void Chuck_VM_Object::init_ref()
{
    // set reference count
    m_ref_count = 0;
    // set flag
    m_pooled = FALSE;
    // set to not locked
    m_locked = FALSE;
    // set v ref
    m_v_ref = NULL;
    // add to vm allocator
    // Chuck_VM_Alloc::instance()->add_object( this );
}




//-----------------------------------------------------------------------------
// name: add_ref()
// desc: add reference
//-----------------------------------------------------------------------------
void Chuck_VM_Object::add_ref()
{
    // increment reference count
    m_ref_count++;

    // if going from 0 to 1
    if( m_ref_count == 1 )
    {
        // add to vm allocator
        Chuck_VM_Alloc::instance()->add_object( this );
    }
}




//-----------------------------------------------------------------------------
// name: release()
// desc: remove reference
//-----------------------------------------------------------------------------
void Chuck_VM_Object::release()
{
    // make sure there is at least one reference
    assert( m_ref_count > 0 );
    // decrement
    m_ref_count--;
    
    // if no more references
    if( m_ref_count == 0 )
    {
        // this is not good
        if( our_locks_in_effect && m_locked )
        {
            EM_error2( 0, "internal error: releasing locked VM object!" );
            // fail
            assert( FALSE );
            // in case assert is disabled
            *(int *)0 = 1;
        }

        // tell the object manager to set this free
        Chuck_VM_Alloc::instance()->free_object( this );
    }
}




//-----------------------------------------------------------------------------
// name: lock()
// desc: lock to keep from deleted
//-----------------------------------------------------------------------------
void Chuck_VM_Object::lock()
{
    m_locked = TRUE;
}




//-----------------------------------------------------------------------------
// name: lock_all()
// desc: disallow deletion of locked objects
//-----------------------------------------------------------------------------
void Chuck_VM_Object::lock_all()
{
    // log
    EM_log( CK_LOG_SYSTEM, "locking down special objects..." );
    // set flag
    our_locks_in_effect = TRUE;
}




//-----------------------------------------------------------------------------
// name: unlock_all()
// desc: allow deletion of locked objects (USE WITH CAUTION!)
//-----------------------------------------------------------------------------
void Chuck_VM_Object::unlock_all()
{
    // log
    EM_log( CK_LOG_SYSTEM, "unprotecting special objects..." );
    // set flag
    our_locks_in_effect = FALSE;
}




// static member
Chuck_VM_Alloc * Chuck_VM_Alloc::our_instance = NULL;


//-----------------------------------------------------------------------------
// name: instance()
// desc: return static instance
//-----------------------------------------------------------------------------
Chuck_VM_Alloc * Chuck_VM_Alloc::instance()
{
    if( !our_instance )
    {
        our_instance = new Chuck_VM_Alloc;
        assert( our_instance != NULL );
    }
    
    return our_instance;
}




//-----------------------------------------------------------------------------
// name: add_object()
// desc: add newly allocated vm object
//-----------------------------------------------------------------------------
void Chuck_VM_Alloc::add_object( Chuck_VM_Object * obj )
{
    // do log
    if( DO_LOG( CK_LOG_CRAZY ) )
    {
        // log it
        EM_log( CK_LOG_CRAZY, "adding '%s' (0x%lx)...",
            mini_type( typeid(*obj).name() ), obj );
    }

    // add it to map
}




//-----------------------------------------------------------------------------
// name: free_object()
// desc: free vm object - reference count should be 0
//-----------------------------------------------------------------------------
void Chuck_VM_Alloc::free_object( Chuck_VM_Object * obj )
{
    // make sure the ref count is 0
    assert( obj && obj->m_ref_count == 0 );

    // do log
    if( DO_LOG( CK_LOG_FINEST ) )
    {
        // log it
        EM_log( CK_LOG_FINEST, "freeing '%s' (0x%lx)...",
            mini_type( typeid(*obj).name() ), obj );
    }

    // remove it from map

    // delete it
    delete obj;
}




//-----------------------------------------------------------------------------
// name: Chuck_VM_Alloc()
// desc: constructor
//-----------------------------------------------------------------------------
Chuck_VM_Alloc::Chuck_VM_Alloc()
{ }




//-----------------------------------------------------------------------------
// name: ~Chuck_VM_Alloc()
// desc: destructor
//-----------------------------------------------------------------------------
Chuck_VM_Alloc::~Chuck_VM_Alloc()
{ }




//-----------------------------------------------------------------------------
// name: Chuck_Object()
// desc: constructor
//-----------------------------------------------------------------------------
Chuck_Object::Chuck_Object()
{
    // zero virtual table
    vtable = NULL;
    // zero type
    type_ref = NULL;
    // zero size
    size = 0;
    // zero data
    data = NULL;

    // add to vm allocator
    Chuck_VM_Alloc::instance()->add_object( this );
}




//-----------------------------------------------------------------------------
// name: Chuck_Object()
// desc: ...
//-----------------------------------------------------------------------------
Chuck_Object::~Chuck_Object()
{
    // free
    if( vtable ) { delete vtable; vtable = NULL; }
    if( type_ref ) { type_ref->release(); type_ref = NULL; }
    if( data ) { delete [] data; size = 0; data = NULL; }
}




//-----------------------------------------------------------------------------
// name: Chuck_Array4()
// desc: constructor
//-----------------------------------------------------------------------------
Chuck_Array4::Chuck_Array4( t_CKBOOL is_obj, t_CKINT capacity )
{
    // sanity check
    assert( capacity >= 0 );
    // set size
    m_vector.resize( capacity );
    // clear (as non-object, so no releases)
    m_is_obj = FALSE;
    this->zero( 0, m_vector.capacity() );
    // is object (set after clear)
    m_is_obj = is_obj;
}




//-----------------------------------------------------------------------------
// name: ~Chuck_Array4()
// desc: destructor
//-----------------------------------------------------------------------------
Chuck_Array4::~Chuck_Array4()
{
    // do nothing
}




//-----------------------------------------------------------------------------
// name: addr()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_Array4::addr( t_CKINT i )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // get the addr
    return (t_CKUINT)(&m_vector[i]);
}




//-----------------------------------------------------------------------------
// name: addr()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_Array4::addr( const string & key )
{
    // get the addr
    return (t_CKUINT)(&m_map[key]);
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::get( t_CKINT i, t_CKUINT * val )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // get the value
    *val = m_vector[i];

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::get( const string & key, t_CKUINT * val )
{
    // set to zero
    *val = 0;
    // find
    map<string, t_CKUINT>::iterator iter = m_map.find( key );
    // check
    if( iter != m_map.end() ) *val = (*iter).second;

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: set()
// desc: include ref counting
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::set( t_CKINT i, t_CKUINT val )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    t_CKUINT v = m_vector[i];

    // if obj
    if( m_is_obj && v ) ((Chuck_Object *)v)->release();

    // set the value
    m_vector[i] = val;

    // if obj
    if( m_is_obj && val ) ((Chuck_Object *)val)->add_ref();

    // return good
    return 1;
}





//-----------------------------------------------------------------------------
// name: set()
// desc: include ref counting
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::set( const string & key, t_CKUINT val )
{
    map<string, t_CKUINT>::iterator iter = m_map.find( key );

    // if obj
    if( m_is_obj && iter != m_map.end() ) 
        ((Chuck_Object *)(*iter).second)->release();

    if( !val ) m_map.erase( key );
    else m_map[key] = val;

    // if obj
    if( m_is_obj && val ) ((Chuck_Object *)val)->add_ref();

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: find()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::find( const string & key )
{
    return m_map.find( key ) != m_map.end();
}




//-----------------------------------------------------------------------------
// name: erase()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::erase( const string & key )
{
    map<string, t_CKUINT>::iterator iter = m_map.find( key );
    t_CKINT v = iter != m_map.end();

    // if obj
    if( m_is_obj && iter != m_map.end() ) 
        ((Chuck_Object *)(*iter).second)->release();

    // erase
    if( v ) m_map.erase( key );

    return v;
}




//-----------------------------------------------------------------------------
// name: push_back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::push_back( t_CKUINT val )
{
    // TODO: release reference or assume 0?

    // add to vector
    m_vector.push_back( val );

    return 1;
}




//-----------------------------------------------------------------------------
// name: pop_back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::pop_back( )
{
    // check
    if( m_vector.size() == 0 )
        return 0;

    // if obj
    if( m_is_obj )
    {
        Chuck_Object * v = (Chuck_Object *)m_vector[m_vector.size()-1];
        if( v ) v->release();
    }
    
    // zero
    m_vector[m_vector.size()-1] = 0;
    // add to vector
    m_vector.pop_back();
    
    return 1;
}




//-----------------------------------------------------------------------------
// name: back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::back( t_CKUINT * val ) const
{
    // check
    if( m_vector.size() == 0 )
        return 0;

    // get
    *val = m_vector.back();
    
    return 1;
}




//-----------------------------------------------------------------------------
// name: clear()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_Array4::clear( )
{
    // zero
    zero( 0, m_vector.size() );

    // clear vector
    // m_vector.clear();
}




//-----------------------------------------------------------------------------
// name: set_capacity()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::set_capacity( t_CKINT capacity )
{
    // sanity check
    assert( capacity >= 0 );

    // ensure size
    set_size( capacity );

    return m_vector.capacity();
}




//-----------------------------------------------------------------------------
// name: set_size()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array4::set_size( t_CKINT size )
{
    // sanity check
    assert( size >= 0 );

    // if clearing size
    if( size < m_vector.size() )
    {
        // zero out section
        zero( size, m_vector.size() );
    }

    // what the size was
    t_CKINT size2 = m_vector.size();
    // resize vector
    m_vector.resize( size );

    // if clearing size
    if( m_vector.size() > size2 )
    {
        // zero out section
        zero( size2, m_vector.size() );
    }

    return m_vector.size();
}




//-----------------------------------------------------------------------------
// name: zero()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_Array4::zero( t_CKUINT start, t_CKUINT end )
{
    // sanity check
    assert( start <= m_vector.capacity() && end <= m_vector.capacity() );

    // if contains objects
    if( m_is_obj )
    {
        Chuck_Object * v = NULL;
        for( t_CKUINT i = start; i < end; i++ )
        {
            // get it
            v = (Chuck_Object *)m_vector[i];
            // release
            if( v )
            {
                v->release();
                m_vector[i] = 0;
            }
        }
    }
    else
    {
        for( t_CKUINT i = start; i < end; i++ )
        {
            // zero
            m_vector[i] = 0;
        }
    }
}




//-----------------------------------------------------------------------------
// name: Chuck_Array8()
// desc: constructor
//-----------------------------------------------------------------------------
Chuck_Array8::Chuck_Array8( t_CKINT capacity )
{
    // sanity check
    assert( capacity >= 0 );
    // set size
    m_vector.resize( capacity );
    // clear
    this->zero( 0, m_vector.capacity() );
}




//-----------------------------------------------------------------------------
// name: ~Chuck_Array8()
// desc: destructor
//-----------------------------------------------------------------------------
Chuck_Array8::~Chuck_Array8()
{
    // do nothing
}




//-----------------------------------------------------------------------------
// name: addr()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_Array8::addr( t_CKINT i )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // get the addr
    return (t_CKUINT)(&m_vector[i]);
}




//-----------------------------------------------------------------------------
// name: addr()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_Array8::addr( const string & key )
{
    // get the addr
    return (t_CKUINT)(&m_map[key]);
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::get( t_CKINT i, t_CKFLOAT * val )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // get the value
    *val = m_vector[i];

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::get( const string & key, t_CKFLOAT * val )
{
    // set to zero
    *val = 0.0;

    // iterator
    map<string, t_CKFLOAT>::iterator iter = m_map.find( key );

    // check
    if( iter != m_map.end() )
    {
        // get the value
        *val = (*iter).second;
    }

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::set( t_CKINT i, t_CKFLOAT val )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // set the value
    m_vector[i] = val;

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::set( const string & key, t_CKFLOAT val )
{
    map<string, t_CKFLOAT>::iterator iter = m_map.find( key );

    if( !val ) m_map.erase( key );
    else m_map[key] = val;

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::find( const string & key )
{
    return m_map.find( key ) != m_map.end();
}



//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::erase( const string & key )
{
    return m_map.erase( key );
}




//-----------------------------------------------------------------------------
// name: push_back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::push_back( t_CKFLOAT val )
{
    // add to vector
    m_vector.push_back( val );

    return 1;
}




//-----------------------------------------------------------------------------
// name: pop_back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::pop_back( )
{
    // check
    if( m_vector.size() == 0 )
        return 0;

    // zero
    m_vector[m_vector.size()-1] = 0.0;
    // add to vector
    m_vector.pop_back();
    
    return 1;
}




//-----------------------------------------------------------------------------
// name: back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::back( t_CKFLOAT * val ) const
{
    // check
    if( m_vector.size() == 0 )
        return 0;

    // get
    *val = m_vector.back();
    
    return 1;
}




//-----------------------------------------------------------------------------
// name: clear()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_Array8::clear( )
{
    // zero
    zero( 0, m_vector.size() );

    // clear vector
    // m_vector.clear();
}




//-----------------------------------------------------------------------------
// name: set_capacity()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::set_capacity( t_CKINT capacity )
{
    // sanity check
    assert( capacity >= 0 );

    // ensure size
    set_size( capacity );

    return m_vector.capacity();
}




//-----------------------------------------------------------------------------
// name: set_size()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array8::set_size( t_CKINT size )
{
    // sanity check
    assert( size >= 0 );

    // if clearing size
    if( size < m_vector.size() )
    {
        // zero out section
        zero( size, m_vector.size() );
    }

    // what the size was
    t_CKINT size2 = m_vector.size();
    // resize vector
    m_vector.resize( size );

    // if clearing size
    if( m_vector.size() > size2 )
    {
        // zero out section
        zero( size2, m_vector.size() );
    }

    return m_vector.size();
}




//-----------------------------------------------------------------------------
// name: zero()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_Array8::zero( t_CKUINT start, t_CKUINT end )
{
    // sanity check
    assert( start <= m_vector.capacity() && end <= m_vector.capacity() );

    for( t_CKUINT i = start; i < end; i++ )
    {
        // zero
        m_vector[i] = 0.0;
    }
}




//-----------------------------------------------------------------------------
// name: Chuck_Array16()
// desc: constructor
//-----------------------------------------------------------------------------
Chuck_Array16::Chuck_Array16( t_CKINT capacity )
{
    // sanity check
    assert( capacity >= 0 );
    // set size
    m_vector.resize( capacity );
    // clear
    this->zero( 0, m_vector.capacity() );
}




//-----------------------------------------------------------------------------
// name: ~Chuck_Array16()
// desc: destructor
//-----------------------------------------------------------------------------
Chuck_Array16::~Chuck_Array16()
{
    // do nothing
}




//-----------------------------------------------------------------------------
// name: addr()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_Array16::addr( t_CKINT i )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // get the addr
    return (t_CKUINT)(&m_vector[i]);
}




//-----------------------------------------------------------------------------
// name: addr()
// desc: ...
//-----------------------------------------------------------------------------
t_CKUINT Chuck_Array16::addr( const string & key )
{
    // get the addr
    return (t_CKUINT)(&m_map[key]);
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::get( t_CKINT i, t_CKCOMPLEX * val )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // get the value
    *val = m_vector[i];

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: get()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::get( const string & key, t_CKCOMPLEX * val )
{
    // set to zero
    val->re = 0.0;
    val->im = 0.0;

    // iterator
    map<string, t_CKCOMPLEX>::iterator iter = m_map.find( key );

    // check
    if( iter != m_map.end() )
    {
        // get the value
        *val = (*iter).second;
    }

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::set( t_CKINT i, t_CKCOMPLEX val )
{
    // bound check
    if( i < 0 || i >= m_vector.capacity() )
        return 0;

    // set the value
    m_vector[i] = val;

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::set( const string & key, t_CKCOMPLEX val )
{
    map<string, t_CKCOMPLEX>::iterator iter = m_map.find( key );

    if( val.re == 0 && val.im == 0 ) m_map.erase( key );
    else m_map[key] = val;

    // return good
    return 1;
}




//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::find( const string & key )
{
    return m_map.find( key ) != m_map.end();
}



//-----------------------------------------------------------------------------
// name: set()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::erase( const string & key )
{
    return m_map.erase( key );
}




//-----------------------------------------------------------------------------
// name: push_back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::push_back( t_CKCOMPLEX val )
{
    // add to vector
    m_vector.push_back( val );
    
    return 1;
}




//-----------------------------------------------------------------------------
// name: pop_back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::pop_back( )
{
    // check
    if( m_vector.size() == 0 )
        return 0;

    // zero
    m_vector[m_vector.size()-1].re = 0.0;
    m_vector[m_vector.size()-1].im = 0.0;
    // add to vector
    m_vector.pop_back();
    
    return 1;
}




//-----------------------------------------------------------------------------
// name: back()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::back( t_CKCOMPLEX * val ) const
{
    // check
    if( m_vector.size() == 0 )
        return 0;

    // get
    *val = m_vector.back();
    
    return 1;
}




//-----------------------------------------------------------------------------
// name: clear()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_Array16::clear( )
{
    // zero
    zero( 0, m_vector.size() );

    // clear vector
    // m_vector.clear();
}




//-----------------------------------------------------------------------------
// name: set_capacity()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::set_capacity( t_CKINT capacity )
{
    // sanity check
    assert( capacity >= 0 );

    // ensure size
    set_size( capacity );

    return m_vector.capacity();
}




//-----------------------------------------------------------------------------
// name: set_size()
// desc: ...
//-----------------------------------------------------------------------------
t_CKINT Chuck_Array16::set_size( t_CKINT size )
{
    // sanity check
    assert( size >= 0 );

    // if clearing size
    if( size < m_vector.size() )
    {
        // zero out section
        zero( size, m_vector.size() );
    }

    // remember
    t_CKINT size2 = m_vector.size();
    // resize vector
    m_vector.resize( size );

    // if clearing size
    if( m_vector.size() > size2 )
    {
        // zero out section
        zero( size2, m_vector.size() );
    }

    return m_vector.size();
}




//-----------------------------------------------------------------------------
// name: zero()
// desc: ...
//-----------------------------------------------------------------------------
void Chuck_Array16::zero( t_CKUINT start, t_CKUINT end )
{
    // sanity check
    assert( start <= m_vector.capacity() && end <= m_vector.capacity() );

    for( t_CKUINT i = start; i < end; i++ )
    {
        // zero
        m_vector[i].re = 0.0;
        m_vector[i].im = 0.0;
    }
}




// static
t_CKUINT Chuck_Event::our_can_wait = 0;

//-----------------------------------------------------------------------------
// name: signal()
// desc: signal a event/condition variable, shreduling the next waiting shred
//       (if there is one or more)
//-----------------------------------------------------------------------------
void Chuck_Event::signal()
{
    m_queue_lock.acquire();
    if( !m_queue.empty() )
    {
        Chuck_VM_Shred * shred = m_queue.front();
        m_queue.pop();
        m_queue_lock.release();
        Chuck_VM_Shreduler * shreduler = shred->vm_ref->shreduler();
        shred->event = NULL;
        shreduler->remove_blocked( shred );
        shreduler->shredule( shred );
        // push the current time
        t_CKTIME *& sp = (t_CKTIME *&)shred->reg->sp;
        push_( sp, shreduler->now_system );
    }
    else
        m_queue_lock.release();
}




//-----------------------------------------------------------------------------
// name: remove()
// desc: remove a shred from the event queue.
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_Event::remove( Chuck_VM_Shred * shred )
{
    queue<Chuck_VM_Shred *> temp;
    t_CKBOOL removed = FALSE;
    m_queue_lock.acquire();
    while( !m_queue.empty() )
    {
        if( m_queue.front() != shred )
            temp.push( m_queue.front() );
        else {
            shred->event = NULL;
            removed = TRUE;
        }
        m_queue.pop();
    }

    m_queue = temp;
    m_queue_lock.release();
    return removed;
}




//-----------------------------------------------------------------------------
// name: queue_broadcast()
// desc: queue the event to broadcast a event/condition variable, by the owner
//       of the queue
//-----------------------------------------------------------------------------
void Chuck_Event::queue_broadcast()
{
    // TODO: handle multiple VM
    m_queue_lock.acquire();
    if( !m_queue.empty() )
    {
        Chuck_VM_Shred * shred = m_queue.front();
        m_queue_lock.release();
        shred->vm_ref->queue_event( this, 1 );
    }
    else
        m_queue_lock.release();

}




//-----------------------------------------------------------------------------
// name: broadcast()
// desc: broadcast a event/condition variable, shreduling all waiting shreds
//-----------------------------------------------------------------------------
void Chuck_Event::broadcast()
{
    m_queue_lock.acquire();
    while( !m_queue.empty() )
    {
        m_queue_lock.release();
        this->signal();
        m_queue_lock.acquire();
    }
    m_queue_lock.release();
}




//-----------------------------------------------------------------------------
// name: wait()
// desc: cause event/condition variable to block the current shred, putting it
//       on its waiting list, and suspennd the shred from the VM.
//-----------------------------------------------------------------------------
void Chuck_Event::wait( Chuck_VM_Shred * shred, Chuck_VM * vm )
{
    EM_log( CK_LOG_FINE, "shred '%d' wait on event '%x'...", shred->xid, (t_CKUINT)this );
    // make sure the shred info matches the vm
    assert( shred->vm_ref == vm );
    
    Chuck_DL_Return RETURN;
    f_mfun canwaitplease = (f_mfun)this->vtable->funcs[our_can_wait]->code->native_func;
    canwaitplease( this, NULL, &RETURN, shred ); // TODO: check this is right shred
    // RETURN.v_int = 1;

    // see if we can wait
    if( RETURN.v_int )
    {
        // suspend
        shred->is_running = FALSE;

        // add to waiting list
        m_queue_lock.acquire();
        m_queue.push( shred );
        m_queue_lock.release();

        // add event to shred
        assert( shred->event == NULL );
        shred->event = this;

        // add shred to shreduler
        vm->shreduler()->add_blocked( shred );
    }
    else // can't wait
    {
        // push the current time
        t_CKTIME *& sp = (t_CKTIME *&)shred->reg->sp;
        push_( sp, shred->now );
    }
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
Chuck_IO::Chuck_IO()
{ }




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
Chuck_IO::~Chuck_IO()
{ }




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
Chuck_IO_File * Chuck_IO::openFile( const string & path, t_CKINT flags )
{
    return NULL;
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
string Chuck_IO::currentDir()
{
    return "";
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
string Chuck_IO::changeDir( const string & to )
{
    return "";
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO::isFile( const string & path )
{
    return FALSE;
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO::isDir( const string & path )
{
    return FALSE;
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
t_CKINT Chuck_IO::getSize( const string & path )
{
    return 0;
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
string Chuck_IO::baseName( const string & path )
{
    return "";
}




//-----------------------------------------------------------------------------
// name:
// desc:
//-----------------------------------------------------------------------------
void Chuck_IO::getContent( vector<string> & content )
{
    content.clear();
}


    

//-----------------------------------------------------------------------------
// name: Chuck_IO_File()
// desc: constructor
//-----------------------------------------------------------------------------
Chuck_IO_File::Chuck_IO_File()
{
    // zero things out
    m_ready_flags = 0;
    m_flags = 0;
}




//-----------------------------------------------------------------------------
// name: ~Chuck_IO_File()
// desc: destructor
//-----------------------------------------------------------------------------
Chuck_IO_File::~Chuck_IO_File()
{
    // check it
    this->close();
}




//-----------------------------------------------------------------------------
// name: open
// desc: open file from disk
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::open( const string & path, t_CKINT flags )
{
    // close first
    this->close();

    // log
    EM_log( CK_LOG_INFO, "(IO): opening file from disk..." );
    EM_pushlog();
    EM_log( CK_LOG_INFO, "(IO): path: %s", path.c_str() );
    EM_log( CK_LOG_INFO, "(IO): READ: %s WRITE: %s APPEND: %s PLUS: %s",
        flags & Chuck_IO::READ ? "Y" : "N", flags & Chuck_IO::WRITE ? "Y" : "N",
        flags & Chuck_IO::APPEND ? "Y" : "N", flags & Chuck_IO::TRUNCATE ? "Y" : "N",
        flags & Chuck_IO::BINARY ? "Y" : "N" );

    // open modes
    int nMode = 0;

    // construct mode string
    stringstream sout;
    if( flags & Chuck_IO::READ )
    {
        // write it
        sout << "r";
        // set ready
        m_ready_flags |= Chuck_IO::READ;
        // set mode
        nMode |= ios::in;
    }
    if( flags & Chuck_IO::WRITE )
    {
        // write it
        sout << "w";
        // set ready
        m_ready_flags |= Chuck_IO::WRITE;
        // set mode
        nMode |= ios::out;
    }
    if( flags & Chuck_IO::APPEND )
    {
        // write it
        sout << "a";
        // set ready
        m_ready_flags |= Chuck_IO::WRITE | Chuck_IO::APPEND;
        // set mode
        nMode |= ios::out | ios::ate;
    }
    if( flags & Chuck_IO::TRUNCATE )
    {
        // read + write
        if( flags ^ Chuck_IO::WRITE || flags & Chuck_IO::APPEND )
        {
            // error
            EM_error3( "(FileIO): malformed open flag (TRUNCATE)..." );
            EM_error3( "    note: must be used with WRITE, and without APPEND" );
            goto error;
        };

        // write it
        sout << "w";
        // set ready
        m_ready_flags |= Chuck_IO::TRUNCATE;
        // set mode
        nMode |= ios::trunc;
    }
    if( flags & Chuck_IO::BINARY )
    {
        // add it
        m_ready_flags |= Chuck_IO::BINARY;
    }

    // sanity check
    if( sout.str().length() == 0 )
    {
        // error
        EM_error3( "(FileIO): malformed open flag (no operation specified)..." );
        goto error;
    }

    // log
    EM_log( CK_LOG_INFO, "(IO): flag: '%s'", sout.str().c_str() );

    // windows sucks for being creative in the wrong places
#ifdef __PLATFORM_WIN32__
    // if( flags ^ Chuck_IO::TRUNCATE && flags | Chuck_IO::READ ) nMode |= ios::nocreate;
    m_io.open( path.c_str(), nMode );
#else
    m_io.open( path.c_str(), (_Ios_Openmode)nMode );
#endif

    // check for error
    if( !m_io.good() )
    {
        EM_error3( "(FileIO): cannot open file: '%s'", path.c_str() );
        goto error;
    }

    // for write
    if( good2write() )
    {
        // set precision
        setprecision( 6 );
    }

    // set path
    m_path = path;
    // set flags
    m_flags = flags;

    // pop
    EM_poplog();

    return TRUE;

error:

    // pop
    EM_poplog();

    // reset
    m_ready_flags = 0;
    // reset
    m_path = "";

    return FALSE;
}




//-----------------------------------------------------------------------------
// name: close
// desc: close file
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::close()
{
    // check
    if( !m_io.good() )
        return FALSE;

    // log
    EM_log( CK_LOG_INFO, "(IO): closing file '%s'...", m_path.c_str() );
    // close it
    m_io.close();
    m_flags = 0;
    m_ready_flags = 0;
    m_path = "";

    return TRUE;
}




//-----------------------------------------------------------------------------
// name: more()
// desc: is there more to read?
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::more()
{
    return !eof();
}




//-----------------------------------------------------------------------------
// name: eof()
// desc: end of file?
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::eof()
{
    // sanity
    if( !m_io.good() ) return TRUE;
    return !m_io;
}




//-----------------------------------------------------------------------------
// name: good2read()
// desc: ready to read?
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::good2read()
{
    return ( m_io.good() && m_flags & Chuck_IO::READ );
}




//-----------------------------------------------------------------------------
// name: good2write()
// desc: ready for write?
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::good2write()
{
    return ( m_io.good() && m_flags & Chuck_IO::READ );
}




//-----------------------------------------------------------------------------
// name: readInt()
// desc: read next as (ascii) integer
//-----------------------------------------------------------------------------
t_CKINT Chuck_IO_File::readInt()
{
    // sanity
    if( !good2read() ) return 0;

    // read int
    t_CKINT val = 0;
    // TODO: check for EOF?
    m_io >> val;

    return val;
}




//-----------------------------------------------------------------------------
// name: readFloat()
// desc: read next as (ascii) floating point value
//-----------------------------------------------------------------------------
t_CKFLOAT Chuck_IO_File::readFloat()
{
    // sanity
    if( !good2read() ) return 0;

    // read float
    t_CKFLOAT val = 0;
    // TODO: check for EOF?
    m_io >> val;

    return 0;
}




//-----------------------------------------------------------------------------
// name: readString()
// desc: read next as string
//-----------------------------------------------------------------------------
string Chuck_IO_File::readString()
{
    // sanity
    if( !good2read() ) return 0;

    // read string
    string val;
    // TODO: check for EOF?
    m_io >> val;

    return val;
}



    
//-----------------------------------------------------------------------------
// name: readLine()
// desc: read line
//-----------------------------------------------------------------------------
string Chuck_IO_File::readLine()
{
    // sanity
    if( !good2read() ) return 0;

    // read string
    string val;
    // TODO: check for EOF?
    std::getline( m_io, val );

    return val;
}




//-----------------------------------------------------------------------------
// name: writeInt()
// desc: write (ascii) integer
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::writeInt( t_CKINT val )
{
    // sanity
    if( !good2write() ) return FALSE;

    // write it
    m_io << val;

    return m_io.good();
}




//-----------------------------------------------------------------------------
// name: writeFloat()
// desc: write (ascii) floating point value
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::writeFloat( t_CKFLOAT val )
{
    // sanity
    if( !good2write() ) return 0;

    // write it
    m_io << val;

    return m_io.good();
}




//-----------------------------------------------------------------------------
// name: writeString()
// desc: write string
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::writeString( const string & val )
{
    // sanity
    if( !good2read() ) return 0;

    // write it
    m_io << val;

    return m_io.good();
}



    
//-----------------------------------------------------------------------------
// name: writeLine()
// desc: write line
//-----------------------------------------------------------------------------
t_CKBOOL Chuck_IO_File::writeLine( const string & val )
{
    // sanity
    if( !good2read() ) return 0;

    // write it
    m_io << val << endl;
    
    return m_io.good();
}





//-----------------------------------------------------------------------------
// name: read32i()
// desc: read next 32 bits, return as int
//-----------------------------------------------------------------------------
t_CKINT Chuck_IO_File::read32i()
{
    return 0;
}




//-----------------------------------------------------------------------------
// name: read24i()
// desc: read next 24 bits, return as int
//-----------------------------------------------------------------------------
t_CKINT Chuck_IO_File::read24i()
{
    return 0;
}




//-----------------------------------------------------------------------------
// name: read16i()
// desc: read next 16 bits, return as int
//-----------------------------------------------------------------------------
t_CKINT Chuck_IO_File::read16i()
{
    return 0;
}




//-----------------------------------------------------------------------------
// name: read8i()
// desc: return next 8 bits, return as int
//-----------------------------------------------------------------------------
t_CKINT Chuck_IO_File::read8i()
{
    return 0;
}




//-----------------------------------------------------------------------------
// name: read32f()
// desc: return next 32-bits as (binary) single float
//-----------------------------------------------------------------------------
t_CKSINGLE Chuck_IO_File::read32f()
{
    return 0;
}




//-----------------------------------------------------------------------------
// name: read64f()
// desc: return next 64-bits as (binary) double float
//-----------------------------------------------------------------------------
t_CKDOUBLE Chuck_IO_File::read64f()
{
    return 0;
}
