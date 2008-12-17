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
// file: chuck_oo.h
// desc: ...
//
// author: Ge Wang (gewang@cs.princeton.edu)
//         Perry R. Cook (prc@cs.princeton.edu)
// date: Autumn 2002
//-----------------------------------------------------------------------------
#ifndef __CHUCK_OO_H__
#define __CHUCK_OO_H__

#include "chuck_def.h"
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <fstream>


// forward reference
struct Chuck_Type;
struct Chuck_Value;
struct Chuck_Func;
struct Chuck_Namespace;
struct Chuck_Context;
struct Chuck_Env;
struct Chuck_VM_Code;
struct Chuck_VM_Shred;
struct Chuck_VM;
struct Chuck_IO_File;




//-----------------------------------------------------------------------------
// name: struct Chuck_VM_Object
// desc: base vm object
//-----------------------------------------------------------------------------
struct Chuck_VM_Object
{
public:
    Chuck_VM_Object() { this->init_ref(); }
    virtual ~Chuck_VM_Object() { }

public:
    // add reference
    void add_ref();
    // release reference
    void release();
    // lock
    void lock();

public:
    // unlock_all: dis/allow deletion of locked objects
    static void lock_all();
    static void unlock_all();
    static t_CKBOOL our_locks_in_effect;

public:
    t_CKUINT m_ref_count; // reference count
    t_CKBOOL m_pooled; // if true, this allocates from a pool
    t_CKBOOL m_locked; // if true, this should never be deleted

public:
    // where
    std::vector<Chuck_VM_Object *> * m_v_ref;
    
private:
    void init_ref();
};




//-----------------------------------------------------------------------------
// name: struct Chuck_VM_Alloc
// desc: vm object manager
//-----------------------------------------------------------------------------
struct Chuck_VM_Alloc
{
public:
    static Chuck_VM_Alloc * instance();
    
public:
    void add_object( Chuck_VM_Object * obj );
    void free_object( Chuck_VM_Object * obj );

protected:
    static Chuck_VM_Alloc * our_instance;

protected:
    Chuck_VM_Alloc();
    ~Chuck_VM_Alloc();

protected: // data
    std::map<Chuck_VM_Object *, void *> m_objects;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_VTable
// desc: virtual table
//-----------------------------------------------------------------------------
struct Chuck_VTable
{
public:
    std::vector<Chuck_Func *> funcs;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Object
// dsec: base object
//-----------------------------------------------------------------------------
struct Chuck_Object : Chuck_VM_Object
{
public:
    Chuck_Object();
    virtual ~Chuck_Object();

public:
    // virtual table
    Chuck_VTable * vtable;
    // reference to type
    Chuck_Type * type_ref;
    // the size of the data region
    t_CKUINT size;
    // data for the object
    t_CKBYTE * data;
};




#define CHUCK_ARRAY4_DATASIZE 4
#define CHUCK_ARRAY8_DATASIZE 8
#define CHUCK_ARRAY16_DATASIZE 16
//-----------------------------------------------------------------------------
// name: struct Chuck_Array
// desc: native ChucK arrays ( virtual base class )
//-----------------------------------------------------------------------------
struct Chuck_Array : Chuck_Object
{
    // functionality that we can keep in common...

public:
    // Chuck_Array();
    // virtual ~Chuck_Array() { }

    virtual t_CKINT size( ) = 0; // const { return m_size; } // array size
    virtual t_CKINT capacity( ) = 0; // const { return m_capacity; } // array capacity
    virtual t_CKINT set_size( t_CKINT size ) = 0; // array size
    virtual t_CKINT set_capacity( t_CKINT capacity ) = 0; // set
    virtual t_CKINT data_type_size( ) = 0; // size of stored type ( from type_ref )
    virtual t_CKINT find( const std::string & key ) = 0; // find
    virtual t_CKINT erase( const std::string & key ) = 0; // erase
    virtual void clear( ) = 0; // clear
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Array4
// desc: native ChucK arrays (for 4-byte)
//-----------------------------------------------------------------------------
struct Chuck_Array4 : Chuck_Array
{
public:
    Chuck_Array4( t_CKBOOL is_obj, t_CKINT capacity = 8 );
    virtual ~Chuck_Array4();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKUINT * val );
    t_CKINT get( const std::string & key, t_CKUINT * val );
    t_CKINT set( t_CKINT i, t_CKUINT val );
    t_CKINT set( const std::string & key, t_CKUINT val );
    t_CKINT push_back( t_CKUINT val );
    t_CKINT pop_back( );
    t_CKINT back( t_CKUINT * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY4_DATASIZE; } 

public:
    std::vector<t_CKUINT> m_vector;
    std::map<std::string, t_CKUINT> m_map;
    t_CKBOOL m_is_obj;
    // t_CKINT m_size;
    // t_CKINT m_capacity;
};




//-----------------------------------------------------------------------------
// name: struct Chuck_Array8
// desc: native ChucK arrays (for 8-byte)
//-----------------------------------------------------------------------------
struct Chuck_Array8 : Chuck_Array
{
public:
    Chuck_Array8( t_CKINT capacity = 8 );
    virtual ~Chuck_Array8();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKFLOAT * val );
    t_CKINT get( const std::string & key, t_CKFLOAT * val );
    t_CKINT set( t_CKINT i, t_CKFLOAT val );
    t_CKINT set( const std::string & key, t_CKFLOAT val );
    t_CKINT push_back( t_CKFLOAT val );
    t_CKINT pop_back( );
    t_CKINT back( t_CKFLOAT * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY8_DATASIZE; }

public:
    std::vector<t_CKFLOAT> m_vector;
    std::map<std::string, t_CKFLOAT> m_map;
    // t_CKINT m_size;
    // t_CKINT m_capacity;
};



//-----------------------------------------------------------------------------
// name: struct Chuck_Array16
// desc: native ChucK arrays (for 16-byte)
//-----------------------------------------------------------------------------
struct Chuck_Array16 : Chuck_Array
{
public:
    Chuck_Array16( t_CKINT capacity = 8 );
    virtual ~Chuck_Array16();

public:
    t_CKUINT addr( t_CKINT i );
    t_CKUINT addr( const std::string & key );
    t_CKINT get( t_CKINT i, t_CKCOMPLEX * val );
    t_CKINT get( const std::string & key, t_CKCOMPLEX * val );
    t_CKINT set( t_CKINT i, t_CKCOMPLEX val );
    t_CKINT set( const std::string & key, t_CKCOMPLEX val );
    t_CKINT push_back( t_CKCOMPLEX val );
    t_CKINT pop_back( );
    t_CKINT back( t_CKCOMPLEX * val ) const;
    void    zero( t_CKUINT start, t_CKUINT end );

    virtual void    clear( );
    virtual t_CKINT size( ) { return m_vector.size(); }
    virtual t_CKINT capacity( ) { return m_vector.capacity(); }
    virtual t_CKINT set_size( t_CKINT size );
    virtual t_CKINT set_capacity( t_CKINT capacity );
    virtual t_CKINT find( const std::string & key );
    virtual t_CKINT erase( const std::string & key );
    virtual t_CKINT data_type_size( ) { return CHUCK_ARRAY16_DATASIZE; }

public:
    std::vector<t_CKCOMPLEX> m_vector;
    std::map<std::string, t_CKCOMPLEX> m_map;
    // t_CKINT m_size;
    // t_CKINT m_capacity;
};




#include "util_thread.h"
//-----------------------------------------------------------------------------
// name: Chuck_Event
// desc: base Chuck Event class
//-----------------------------------------------------------------------------
struct Chuck_Event : Chuck_Object
{
public:
    void signal();
    void broadcast();
    void wait( Chuck_VM_Shred * shred, Chuck_VM * vm );
    t_CKBOOL remove( Chuck_VM_Shred * shred );

public: // internal
    void queue_broadcast();
    static t_CKUINT our_can_wait;

    std::queue<Chuck_VM_Shred *> m_queue;
    XMutex m_queue_lock;
};




//-----------------------------------------------------------------------------
// name: Chuck_String
// desc: base Chuck string class
//-----------------------------------------------------------------------------
struct Chuck_String : Chuck_Object
{
public:
    Chuck_String( const std::string & s = "" ) { str = s; }
    ~Chuck_String() { }

public:
    std::string str;
};




//-----------------------------------------------------------------------------
// name: Chuck_IO
// desc: base Chuck IO class
//-----------------------------------------------------------------------------
struct Chuck_IO : Chuck_Object
{
public:
    virtual ~Chuck_IO();

public:
    // query
    virtual t_CKBOOL more() = 0;
    virtual t_CKBOOL eof() = 0;
    virtual t_CKBOOL good2read() = 0;
    virtual t_CKBOOL good2write() = 0;

    // ascii
    virtual t_CKINT readInt() = 0;
    virtual t_CKFLOAT readFloat() = 0;
    virtual std::string readString() = 0;
    virtual std::string readLine() = 0;
    virtual t_CKINT writeInt() = 0;
    virtual t_CKFLOAT writeFloat() = 0;
    virtual std::string writeString() = 0;
    virtual std::string writeLine() = 0;
    
    // binary
    virtual t_CKINT read32i() = 0;
    virtual t_CKINT read24i() = 0;
    virtual t_CKINT read16i() = 0;
    virtual t_CKINT read8i() = 0;
    virtual t_CKSINGLE read32f() = 0;
    virtual t_CKDOUBLE read64f() = 0;

    // done
    virtual t_CKBOOL close() = 0;

// constants
public:
    static const t_CKINT READ;
    static const t_CKINT WRITE;
    static const t_CKINT APPEND;
    static const t_CKINT TRUNCATE;
    static const t_CKINT ASCII;
    static const t_CKINT BINARY;

// static utilities
public:
    static Chuck_IO_File * openFile( const std::string & path, t_CKINT flags );

    // more stuff
    static t_CKBOOL isFile( const std::string & path );
    static t_CKBOOL isDir( const std::string & path );
    static t_CKINT getSize( const std::string & path );
    static std::string currentDir();
    static std::string changeDir( const std::string & to );
    static void getContent( std::vector<std::string> & content );
    static std::string baseName( const std::string & path );

// can't instantiate one of these
protected:
    Chuck_IO();
};




//-----------------------------------------------------------------------------
// name: Chuck_IO_File
// desc: Chuck File IO class
//-----------------------------------------------------------------------------
struct Chuck_IO_File : Chuck_IO
{
public:
    Chuck_IO_File();
    virtual ~Chuck_IO_File();

public:
    // open
    t_CKBOOL open( const std::string & path, t_CKINT flags );
    t_CKBOOL close();

    // query
    virtual t_CKBOOL more();
    virtual t_CKBOOL eof();
    virtual t_CKBOOL good2read();
    virtual t_CKBOOL good2write();

    // ascii
    virtual t_CKINT readInt();
    virtual t_CKFLOAT readFloat();
    virtual std::string readString();
    virtual std::string readLine();
    virtual t_CKBOOL writeInt( t_CKINT val );
    virtual t_CKBOOL writeFloat( t_CKFLOAT val );
    virtual t_CKBOOL writeString( const std::string & val );
    virtual t_CKBOOL writeLine( const std::string & val );
    
    // binary
    virtual t_CKINT read32i();
    virtual t_CKINT read24i();
    virtual t_CKINT read16i();
    virtual t_CKINT read8i();
    virtual t_CKSINGLE read32f();
    virtual t_CKDOUBLE read64f();

protected:
    // open floags
    t_CKINT m_flags;
    // ready flags
    t_CKINT m_ready_flags;
    // in file
    std::fstream m_io;
    // path
    std::string m_path;
};




#endif
