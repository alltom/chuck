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
// name: chuck_lang.cpp
// desc: chuck class library base
//
// authors: Ge Wang (gewang@cs.princeton.edu)
//          Perry R. Cook (prc@cs.princeton.edu)
//          Spencer Salazar (ssalazar@cs.princeton.edu)
//          Andrew Schran (aschran@princeton.edu)
//    date: spring 2005
//-----------------------------------------------------------------------------
#include "chuck_lang.h"
#include "chuck_type.h"
#include "chuck_instr.h"
#include "chuck_vm.h"
#include "chuck_errmsg.h"
#include "chuck_ugen.h"
#include "chuck_globals.h"
#include "hidio_sdl.h"
#include "util_string.h"

#ifndef __DISABLE_MIDI__
#include "midiio_rtmidi.h"
#endif

#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

// dac tick
CK_DLL_TICK(__ugen_tick) { *out = in; return TRUE; }
// object string offset
static t_CKUINT Object_offset_string = 0;

// global
static Chuck_String * g_newline = new Chuck_String();




//-----------------------------------------------------------------------------
// name: init_class_object()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_object( Chuck_Env * env, Chuck_Type * type )
{
    Chuck_DL_Func * func = NULL;

    // log
    EM_log( CK_LOG_SEVERE, "class 'object'" );

    // init as base class
    if( !type_engine_import_class_begin( env, type, env->global(), object_ctor, object_dtor ) )
        return FALSE;

    // add member to hold string
    Object_offset_string = type_engine_import_mvar( env, "string", "@string", FALSE );
    if( Object_offset_string == CK_INVALID_OFFSET ) goto error;

    // add toString()
    func = make_new_mfun( "string", "toString", object_toString );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // end the class import
    type_engine_import_class_end( env );
    
    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}




//-----------------------------------------------------------------------------
// name: init_class_ugen()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_ugen( Chuck_Env * env, Chuck_Type * type )
{
    Chuck_DL_Func * func = NULL;

    EM_log( CK_LOG_SEVERE, "class 'ugen'" );

    // add ugen info
    type->ugen_info = new Chuck_UGen_Info;
    type->ugen_info->add_ref();
    type->ugen_info->tick = __ugen_tick;
    type->ugen_info->num_ins = 1;
    type->ugen_info->num_outs = 1;

    // init as base class
    // TODO: ctor/dtor, ugen's sometimes created internally?
    if( !type_engine_import_class_begin( env, type, env->global(), NULL, NULL ) )
        return FALSE;

    // gain
    func = make_new_mfun( "float", "gain", ugen_gain );
    func->add_arg( "float", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    func = make_new_mfun( "float", "gain", ugen_cget_gain );
    if( !type_engine_import_mfun( env, func ) ) goto error;

/*    // next
    func = make_new_mfun( "float", "next", ugen_next );
    func->add_arg( "float", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    func = make_new_mfun( "float", "next", ugen_cget_next );
    if( !type_engine_import_mfun( env, func ) ) goto error;
*/
    // last
    func = make_new_mfun( "float", "last", ugen_last );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // op
    func = make_new_mfun( "int", "op", ugen_op );
    func->add_arg( "int", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    func = make_new_mfun( "int", "op", ugen_cget_op );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add numChannels
    func = make_new_mfun( "int", "channels", ugen_numChannels );
    func->add_arg( "int", "num" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    func = make_new_mfun( "int", "channels", ugen_cget_numChannels );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add chan
    func = make_new_mfun( "UGen", "chan", ugen_chan );
    func->add_arg( "int", "num" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add isConnectedTo
    func = make_new_mfun( "int", "isConnectedTo", ugen_connected );
    func->add_arg( "UGen", "right" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // end
    type_engine_import_class_end( env );

    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );

    return FALSE;
}




// virtual table offset
static t_CKINT uana_offset_blob = 0;
//-----------------------------------------------------------------------------
// name: init_class_uana()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_uana( Chuck_Env * env, Chuck_Type * type )
{
    Chuck_DL_Func * func = NULL;

    EM_log( CK_LOG_SEVERE, "class 'uana'" );

    // add uana info
    type->ugen_info = new Chuck_UGen_Info;
    type->ugen_info->add_ref();
    type->ugen_info->tick = __ugen_tick;
    type->ugen_info->num_ins = 1;
    type->ugen_info->num_outs = 1;

    // init as base class, type should already know the parent type
    // TODO: ctor/dtor, ugen's sometimes created internally?
    if( !type_engine_import_class_begin( env, type, env->global(), uana_ctor, uana_dtor ) )
        return FALSE;

    // add variables
    uana_offset_blob = type_engine_import_mvar( env, "UAnaBlob", "m_blob", FALSE );
    if( uana_offset_blob == CK_INVALID_OFFSET ) goto error;

    // add upchuck
    func = make_new_mfun( "UAnaBlob", "upchuck", uana_upchuck );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add fvals
    func = make_new_mfun( "float[]", "fvals", uana_fvals );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add cvals
    func = make_new_mfun( "complex[]", "cvals", uana_cvals );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add fval
    func = make_new_mfun( "float", "fval", uana_fval );
    func->add_arg( "int", "index" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add cval
    func = make_new_mfun( "complex", "cval", uana_cval );
    func->add_arg( "int", "index" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add isUpConnectedTo
    func = make_new_mfun( "int", "isUpConnectedTo", uana_connected );
    func->add_arg( "UAna", "right" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // TODO: add nonchuck
    // func = make_new_mfun( "void", "nonchuck", uana_nonchuck );
    // if( !type_engine_import_mfun( env, func ) ) goto error;

    // TODO: add event
    // func = make_new_mfun( "Event", "event", uana_event );
    // if( !type_engine_import_mfun( env, func ) ) goto error;

    // end
    type_engine_import_class_end( env );

    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );

    return FALSE;
}




// virtual table offset
static t_CKINT uanablob_offset_when = 0;
static t_CKINT uanablob_offset_fvals = 0;
static t_CKINT uanablob_offset_cvals = 0;
//-----------------------------------------------------------------------------
// name: init_class_blob()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_blob( Chuck_Env * env, Chuck_Type * type )
{
    Chuck_DL_Func * func = NULL;
    Chuck_Value * value = NULL;
    
    // log
    EM_log( CK_LOG_SEVERE, "class 'uanablob'" );
    
    // init class
    // TODO: ctor/dtor
    if( !type_engine_import_class_begin( env, type, env->global(), uanablob_ctor, uanablob_dtor ) )
        return FALSE;

    // add variables
    uanablob_offset_when = type_engine_import_mvar( env, "time", "m_when", FALSE );
    if( uanablob_offset_when == CK_INVALID_OFFSET ) goto error;
    uanablob_offset_fvals = type_engine_import_mvar( env, "float[]", "m_fvals", FALSE );
    if( uanablob_offset_fvals == CK_INVALID_OFFSET ) goto error;
    uanablob_offset_cvals = type_engine_import_mvar( env, "complex[]", "m_cvals", FALSE );
    if( uanablob_offset_cvals == CK_INVALID_OFFSET ) goto error;

    // add when
    func = make_new_mfun( "time", "when", uanablob_when );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add fvals
    func = make_new_mfun( "float[]", "fvals", uanablob_fvals );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add cvals
    func = make_new_mfun( "complex[]", "cvals", uanablob_cvals );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add fval
    func = make_new_mfun( "float", "fval", uanablob_fval );
    func->add_arg( "int", "index" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add cval
    func = make_new_mfun( "complex", "cval", uanablob_cval );
    func->add_arg( "int", "index" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // end class import
    type_engine_import_class_end( env );
    
    // done
    return TRUE;

error:

    // end class import
    type_engine_import_class_end( env );
    
    // error
    return FALSE;
}




// static t_CKUINT event_offset_data = 0;
//-----------------------------------------------------------------------------
// name: init_class_event()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_event( Chuck_Env * env, Chuck_Type * type )
{
    Chuck_DL_Func * func = NULL;
    Chuck_Value * value = NULL;

    // log
    EM_log( CK_LOG_SEVERE, "class 'event'" );

    // init as base class
    // TODO: ctor/dtor?
    if( !type_engine_import_class_begin( env, type, env->global(), NULL, NULL ) )
        return FALSE;

    // add signal()
    func = make_new_mfun( "void", "signal", event_signal );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add broadcast()
    func = make_new_mfun( "void", "broadcast", event_broadcast );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add wait()
    func = make_new_mfun( "void", "wait", event_wait );
    func->add_arg( "Shred", "me" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add can_wait()
    func = make_new_mfun( "int", "can_wait", event_can_wait );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // end the class import
    type_engine_import_class_end( env );

    // find the offset for can_wait
    value = type_engine_find_value( type, "can_wait" );
    assert( value != NULL );
    assert( value->func_ref != NULL );
    // remember it
    Chuck_Event::our_can_wait = value->func_ref->vt_index;
    
    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}




static t_CKUINT shred_offset_args = 0;
//-----------------------------------------------------------------------------
// name: init_class_shred()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_shred( Chuck_Env * env, Chuck_Type * type )
{
    // init as base class
    Chuck_DL_Func * func = NULL;

    // log
    EM_log( CK_LOG_SEVERE, "class 'shred'" );

    // init as base class
    // TODO: ctor/dtor?
    if( !type_engine_import_class_begin( env, type, env->global(), NULL, NULL ) )
        return FALSE;
    
    // add dtor
    // not

    // add clone()
    func = make_new_mfun( "void", "clone", shred_clone );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add exit()
    func = make_new_mfun( "void", "exit", shred_exit );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add id()
    func = make_new_mfun( "int", "id", shred_id );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add yield()
    func = make_new_mfun( "void", "yield", shred_yield );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add running()
    func = make_new_mfun( "int", "running", shred_running );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add done()
    func = make_new_mfun( "int", "done", shred_done );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add nargs()
    // func = make_new_mfun( "int", "numArgs", shred_numArgs );
    // if( !type_engine_import_mfun( env, func ) ) goto error;

    // add nargs()
    func = make_new_mfun( "int", "args", shred_numArgs );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add arg()
    // func = make_new_mfun( "string", "getArg", shred_getArg );
    // func->add_arg( "int", "index" );
    // if( !type_engine_import_mfun( env, func ) ) goto error;

    // add arg()
    func = make_new_mfun( "string", "arg", shred_getArg );
    func->add_arg( "int", "index" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // end the class import
    type_engine_import_class_end( env );
    
    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}




//-----------------------------------------------------------------------------
// name: init_class_io()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_io( Chuck_Env * env, Chuck_Type * type )
{
    // init as base class
    Chuck_DL_Func * func = NULL;
    
    // log
    EM_log( CK_LOG_SEVERE, "class 'io'" );
    
    // init as base class
    // TODO: ctor/dtor?
    // TODO: replace dummy with pure function
    if( !type_engine_import_class_begin( env, type, env->global(), NULL, NULL ) )
        return FALSE;
    
    // add good()
    func = make_new_mfun( "int", "good", io_dummy );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add close()
    func = make_new_mfun( "void", "close", io_dummy );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add flush()
    func = make_new_mfun( "void", "flush", io_dummy );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add mode(int)
    func = make_new_mfun( "int", "mode", io_dummy );
    func->add_arg( "int", "flag" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add mode()
    func = make_new_mfun( "int", "mode", io_dummy );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
	// TODO: add this later?
    // add read()
    // func = make_new_mfun( "string", "read", io_dummy );
    // func->add_arg( "int", "length" );
    // if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readLine()
    func = make_new_mfun( "string", "readLine", io_dummy );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readInt()
    func = make_new_mfun( "int", "readInt", io_dummy );
    func->add_arg( "int", "flags" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readFloat()
    // func = make_new_mfun( "float", "readFloat", io_dummy );
    // if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add eof()
    func = make_new_mfun( "int", "eof", io_dummy );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add more()
    func = make_new_mfun( "int", "more", io_dummy );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    
    // add write(string)
    func = make_new_mfun( "void", "write", io_dummy );
    func->add_arg( "string", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add write(int)
    func = make_new_mfun( "void", "write", io_dummy );
    func->add_arg( "int", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add write(float)
    func = make_new_mfun( "void", "write", io_dummy );
    func->add_arg( "float", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add newline
    func = make_new_sfun( "string", "newline", io_newline );
    if( !type_engine_import_sfun( env, func ) ) goto error;
    func = make_new_sfun( "string", "nl", io_newline );
    if( !type_engine_import_sfun( env, func ) ) goto error;
    func = make_new_sfun( "string", "newlineEx2VistaHWNDVisualFoxProA", io_newline );
    if( !type_engine_import_sfun( env, func ) ) goto error;
    initialize_object( g_newline, &t_string );
    g_newline->str = "\n";
    
    // add READ_INT32
    if( !type_engine_import_svar( env, "int", "READ_INT32",
                                 TRUE, (t_CKUINT)&Chuck_IO::READ_INT32 ) ) goto error;
    
    // add READ_INT16
    if( !type_engine_import_svar( env, "int", "READ_INT16",
                                 TRUE, (t_CKUINT)&Chuck_IO::READ_INT16 ) ) goto error;
    
    // add READ_INT8
    if( !type_engine_import_svar( env, "int", "READ_INT8",
                                 TRUE, (t_CKUINT)&Chuck_IO::READ_INT8 ) ) goto error;
    
    // add MODE_SYNC
    if( !type_engine_import_svar( env, "int", "MODE_SYNC",
                                 TRUE, (t_CKUINT)&Chuck_IO::MODE_SYNC ) ) goto error;
    
    // add MODE_ASYNC
    if( !type_engine_import_svar( env, "int", "MODE_ASYNC",
                                 TRUE, (t_CKUINT)&Chuck_IO::MODE_ASYNC ) ) goto error;
    
    // end the class import
    type_engine_import_class_end( env );
    
    return TRUE;
    
error:
    
    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}




//-----------------------------------------------------------------------------
// name: init_class_fileio()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_fileio( Chuck_Env * env, Chuck_Type * type )
{
    // init as base class
    Chuck_DL_Func * func = NULL;
    
    // log
    EM_log( CK_LOG_SEVERE, "class 'fileio'" );
    
    // init as base class
    // TODO: ctor/dtor?
    // TODO: replace dummy with pure function
    if( !type_engine_import_class_begin( env, type, env->global(), fileio_ctor, fileio_dtor ) )
        return FALSE;
    
    // add open(string)
    func = make_new_mfun( "int", "open", fileio_open );
    func->add_arg( "string", "path" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add open(string, flags)
    func = make_new_mfun( "int", "open", fileio_openflags );
    func->add_arg( "string", "path" );
    func->add_arg( "int", "flags" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add good()
    func = make_new_mfun( "int", "good", fileio_good );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add close()
    func = make_new_mfun( "void", "close", fileio_close );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add flush()
    func = make_new_mfun( "void", "flush", fileio_flush );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add mode()
    func = make_new_mfun( "int", "mode", fileio_getmode );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add mode(int)
    func = make_new_mfun( "int", "mode", fileio_setmode );
    func->add_arg( "int", "flag" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add size()
    func = make_new_mfun( "int", "size", fileio_size );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add seek(int)
    func = make_new_mfun( "void", "seek", fileio_seek );
    func->add_arg( "int", "pos" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add tell()
    func = make_new_mfun( "int", "tell", fileio_tell );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add isDir()
    func = make_new_mfun( "int", "isDir", fileio_isdir );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add dirList()
    func = make_new_mfun( "string[]", "dirList", fileio_dirlist );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add read()
    // func = make_new_mfun( "string", "read", fileio_read );
    // func->add_arg( "int", "length" );
    // if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readLine()
    func = make_new_mfun( "string", "readLine", fileio_readline );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readInt()
    // func = make_new_mfun( "int", "readInt", fileio_readint );
    // if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readInt(int)
    func = make_new_mfun( "int", "readInt", fileio_readintflags );
    func->add_arg( "int", "flags" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readFloat()
    // func = make_new_mfun( "float", "readFloat", fileio_readfloat );
    // if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add eof()
    func = make_new_mfun( "int", "eof", fileio_eof );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add more()
    func = make_new_mfun( "int", "more", fileio_more );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add write(string)
    func = make_new_mfun( "void", "write", fileio_writestring );
    func->add_arg( "string", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add write(int)
    func = make_new_mfun( "void", "write", fileio_writeint );
    func->add_arg( "int", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add write(float)
    func = make_new_mfun( "void", "write", fileio_writefloat );
    func->add_arg( "float", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add FLAG_READ_WRITE
    if( !type_engine_import_svar( env, "int", "READ_WRITE",
                                  TRUE, (t_CKUINT)&Chuck_IO_File::FLAG_READ_WRITE ) ) goto error;
    
    // add FLAG_READONLY
    if( !type_engine_import_svar( env, "int", "READ",
                                  TRUE, (t_CKUINT)&Chuck_IO_File::FLAG_READONLY ) ) goto error;
    
    // add FLAG_WRITEONLY
    if( !type_engine_import_svar( env, "int", "WRITE",
                                  TRUE, (t_CKUINT)&Chuck_IO_File::FLAG_WRITEONLY ) ) goto error;
    
    // add FLAG_APPEND
    if( !type_engine_import_svar( env, "int", "APPEND",
                                  TRUE, (t_CKUINT)&Chuck_IO_File::FLAG_APPEND ) ) goto error;
    
    // add TYPE_ASCII
    if( !type_engine_import_svar( env, "int", "ASCII",
                                  TRUE, (t_CKUINT)&Chuck_IO_File::TYPE_ASCII ) ) goto error;
    
    // add TYPE_BINARY
    if( !type_engine_import_svar( env, "int", "BINARY",
                                  TRUE, (t_CKUINT)&Chuck_IO_File::TYPE_BINARY ) ) goto error;
    
    // end the class import
    type_engine_import_class_end( env );
    
    return TRUE;
    
error:
    
    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}




//-----------------------------------------------------------------------------
// name: init_class_string()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_string( Chuck_Env * env, Chuck_Type * type )
{
    // init as base class
    Chuck_DL_Func * func = NULL;

    // log
    EM_log( CK_LOG_SEVERE, "class 'string'" );

    // init as base class
    // TODO: ctor/dtor
    if( !type_engine_import_class_begin( env, type, env->global(), NULL, NULL ) )
        return FALSE;

    // add length()
    func = make_new_mfun( "int", "length", string_length );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add upper()
    func = make_new_mfun( "string", "upper", string_upper );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add lower()
    func = make_new_mfun( "string", "lower", string_lower );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add ltrim()
    func = make_new_mfun( "string", "ltrim", string_ltrim );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add rtrim()
    func = make_new_mfun( "string", "rtrim", string_rtrim );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add trim()
    func = make_new_mfun( "string", "trim", string_trim );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add toString()
    func = make_new_mfun( "string", "toString", string_toString );
    if( !type_engine_import_mfun( env, func ) ) goto error;

/*    // add at()
    func = make_new_mfun( "int", "ch", string_set_at );
    func->add_arg( "int", "index" );
    func->add_arg( "int", "val" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    func = make_new_mfun( "int", "ch", string_get_at );
    func->add_arg( "int", "index" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
*/
    // end the class import
    type_engine_import_class_end( env );
    
    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}




//-----------------------------------------------------------------------------
// name: init_class_array()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_array( Chuck_Env * env, Chuck_Type * type )
{
    // init as base class
    Chuck_DL_Func * func = NULL;

    // log
    EM_log( CK_LOG_SEVERE, "class 'array'" );

    // init as base class
    // TODO: ctor/dtor?
    if( !type_engine_import_class_begin( env, type, env->global(), NULL, NULL ) )
        return FALSE;

    // add clear()
    func = make_new_mfun( "void", "clear", array_clear );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add reset()
    func = make_new_mfun( "void", "reset", array_reset );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add popBack()
    func = make_new_mfun( "void", "popBack", array_pop_back );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add size()
    func = make_new_mfun( "int", "size", array_get_size );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    // add size()
    func = make_new_mfun( "int", "size", array_set_size );
    func->add_arg( "int", "newSize" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add cap()
    func = make_new_mfun( "int", "cap", array_get_capacity_hack );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add capacity()
    // func = make_new_mfun( "int", "capacity", array_get_capacity );
    // if( !type_engine_import_mfun( env, func ) ) goto error;
    // func = make_new_mfun( "int", "capacity", array_set_capacity );
    // func->add_arg( "int", "val" );
    // if( !type_engine_import_mfun( env, func ) ) goto error;

    // add find()
    func = make_new_mfun( "int", "find", array_find );
    func->add_arg( "string", "key" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add erase()
    func = make_new_mfun( "int", "erase", array_erase );
    func->add_arg( "string", "key" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    type_engine_import_class_end( env );

    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}



#ifndef __DISABLE_MIDI__

// static
static t_CKUINT MidiIn_offset_data = 0;
static t_CKUINT MidiMsg_offset_data1 = 0;
static t_CKUINT MidiMsg_offset_data2 = 0;
static t_CKUINT MidiMsg_offset_data3 = 0;
static t_CKUINT MidiMsg_offset_when = 0;
static t_CKUINT MidiOut_offset_data = 0;

//-----------------------------------------------------------------------------
// name: init_class_Midi()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_Midi( Chuck_Env * env )
{
    Chuck_DL_Func * func = NULL;

    // init base class
    // TODO: ctor/dtor?
    if( !type_engine_import_class_begin( env, "MidiMsg", "Object",
                                         env->global(), NULL, NULL ) )
        return FALSE;

    // add member variable
    MidiMsg_offset_data1 = type_engine_import_mvar( env, "int", "data1", FALSE );
    if( MidiMsg_offset_data1 == CK_INVALID_OFFSET ) goto error;

    // add member variable
    MidiMsg_offset_data2 = type_engine_import_mvar( env, "int", "data2", FALSE );
    if( MidiMsg_offset_data2 == CK_INVALID_OFFSET ) goto error;

    // add member variable
    MidiMsg_offset_data3 = type_engine_import_mvar( env, "int", "data3", FALSE );
    if( MidiMsg_offset_data3 == CK_INVALID_OFFSET ) goto error;

    // add member variable
    MidiMsg_offset_when = type_engine_import_mvar( env, "time", "when", FALSE );
    if( MidiMsg_offset_when == CK_INVALID_OFFSET ) goto error;

    // end the class import
    type_engine_import_class_end( env );


    // init base class
    if( !type_engine_import_class_begin( env, "MidiIn", "Event",
                                         env->global(), MidiIn_ctor, MidiIn_dtor ) )
        return FALSE;

    // add open()
    func = make_new_mfun( "int", "open", MidiIn_open );
    func->add_arg( "int", "port" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add good()
    func = make_new_mfun( "int", "good", MidiIn_good );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add num()
    func = make_new_mfun( "int", "num", MidiIn_num );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add name()
    func = make_new_mfun( "string", "name", MidiIn_name );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add printerr()
    func = make_new_mfun( "void", "printerr", MidiIn_printerr );
    func->add_arg( "int", "print_or_not" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add recv()
    func = make_new_mfun( "int", "recv", MidiIn_recv );
    func->add_arg( "MidiMsg", "msg" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add can_wait()
    func = make_new_mfun( "int", "can_wait", MidiIn_can_wait );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add member variable
    MidiIn_offset_data = type_engine_import_mvar( env, "int", "@MidiIn_data", FALSE );
    if( MidiIn_offset_data == CK_INVALID_OFFSET ) goto error;

    // end the class import
    type_engine_import_class_end( env );
    
    // init base class
    if( !type_engine_import_class_begin( env, "MidiOut", "Object",
                                         env->global(), MidiOut_ctor, MidiOut_dtor ) )
        return FALSE;

    // add open()
    func = make_new_mfun( "int", "open", MidiOut_open );
    func->add_arg( "int", "port" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add good()
    func = make_new_mfun( "int", "good", MidiOut_good );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add num()
    func = make_new_mfun( "int", "num", MidiOut_num );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add name()
    func = make_new_mfun( "string", "name", MidiOut_name );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add printerr()
    func = make_new_mfun( "void", "printerr", MidiOut_printerr );
    func->add_arg( "int", "print_or_not" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add send()
    func = make_new_mfun( "int", "send", MidiOut_send );
    func->add_arg( "MidiMsg", "msg" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add member variable
    MidiOut_offset_data = type_engine_import_mvar( env, "int", "@MidiOut_data", FALSE );
    if( MidiOut_offset_data == CK_INVALID_OFFSET ) goto error;

    // end the class import
    type_engine_import_class_end( env );
    
    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}

#endif // __DISABLE_MIDI__




// static
static t_CKUINT HidIn_offset_data = 0;

static t_CKUINT HidMsg_offset_device_type = 0;
static t_CKUINT HidMsg_offset_device_num = 0;
static t_CKUINT HidMsg_offset_type = 0;
static t_CKUINT HidMsg_offset_which = 0;
static t_CKUINT HidMsg_offset_idata = 0;
static t_CKUINT HidMsg_offset_fdata = 0;
static t_CKUINT HidMsg_offset_when = 0;
static t_CKUINT HidMsg_offset_deltax = 0;
static t_CKUINT HidMsg_offset_deltay = 0;
static t_CKUINT HidMsg_offset_axis_position = 0; // deprecated
static t_CKUINT HidMsg_offset_axis_position2 = 0;
static t_CKUINT HidMsg_offset_scaled_axis_position = 0; // deprecated
static t_CKUINT HidMsg_offset_hat_position = 0;
static t_CKUINT HidMsg_offset_cursorx = 0;
static t_CKUINT HidMsg_offset_cursory = 0;
static t_CKUINT HidMsg_offset_scaledcursorx = 0;
static t_CKUINT HidMsg_offset_scaledcursory = 0;
static t_CKUINT HidMsg_offset_x = 0;
static t_CKUINT HidMsg_offset_y = 0;
static t_CKUINT HidMsg_offset_z = 0;
static t_CKUINT HidMsg_offset_ascii = 0;
static t_CKUINT HidMsg_offset_key = 0;

static t_CKUINT HidOut_offset_data = 0;

//-----------------------------------------------------------------------------
// name: init_class_HID()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_HID( Chuck_Env * env )
{
    Chuck_DL_Func * func = NULL;

    // init base class
    if( !type_engine_import_class_begin( env, "HidMsg", "Object",
                                         env->global(), NULL, NULL ) )
        return FALSE;

    // add member variable
    HidMsg_offset_device_type = type_engine_import_mvar( env, "int", "deviceType", FALSE );
    if( HidMsg_offset_device_type == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_device_num = type_engine_import_mvar( env, "int", "deviceNum", FALSE );
    if( HidMsg_offset_device_num == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_type = type_engine_import_mvar( env, "int", "type", FALSE );
    if( HidMsg_offset_type == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_which = type_engine_import_mvar( env, "int", "which", FALSE );
    if( HidMsg_offset_which == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_idata = type_engine_import_mvar( env, "int", "idata", FALSE );
    if( HidMsg_offset_idata == CK_INVALID_OFFSET ) goto error;

    // add member variable
    HidMsg_offset_fdata = type_engine_import_mvar( env, "float", "fdata", FALSE );
    if( HidMsg_offset_fdata == CK_INVALID_OFFSET ) goto error;

    // add member variable
    HidMsg_offset_when = type_engine_import_mvar( env, "time", "when", FALSE );
    if( HidMsg_offset_when == CK_INVALID_OFFSET ) goto error;

    // add member variable
    HidMsg_offset_deltax = type_engine_import_mvar( env, "int", "deltaX", FALSE );
    if( HidMsg_offset_deltax == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_deltay = type_engine_import_mvar( env, "int", "deltaY", FALSE );
    if( HidMsg_offset_deltay == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_cursorx = type_engine_import_mvar( env, "int", "cursorX", FALSE );
    if( HidMsg_offset_cursorx == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_cursory = type_engine_import_mvar( env, "int", "cursorY", FALSE );
    if( HidMsg_offset_cursory == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_scaledcursorx = type_engine_import_mvar( env, "float", "scaledCursorX", FALSE );
    if( HidMsg_offset_scaledcursorx == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_scaledcursory = type_engine_import_mvar( env, "float", "scaledCursorY", FALSE );
    if( HidMsg_offset_scaledcursory == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_x = type_engine_import_mvar( env, "int", "x", FALSE );
    if( HidMsg_offset_x == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_y = type_engine_import_mvar( env, "int", "y", FALSE );
    if( HidMsg_offset_y == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_z = type_engine_import_mvar( env, "int", "z", FALSE );
    if( HidMsg_offset_z == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_axis_position = type_engine_import_mvar( env, "int", "axis_position", FALSE );
    if( HidMsg_offset_axis_position == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_axis_position2 = type_engine_import_mvar( env, "float", "axisPosition", FALSE );
    if( HidMsg_offset_axis_position2 == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_hat_position = type_engine_import_mvar( env, "int", "hatPosition", FALSE );
    if( HidMsg_offset_hat_position == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_ascii = type_engine_import_mvar( env, "int", "ascii", FALSE );
    if( HidMsg_offset_ascii == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_key = type_engine_import_mvar( env, "int", "key", FALSE );
    if( HidMsg_offset_key == CK_INVALID_OFFSET ) goto error;
    
    // add member variable
    HidMsg_offset_scaled_axis_position = type_engine_import_mvar( env, "float", "scaled_axis_position", FALSE );
    if( HidMsg_offset_scaled_axis_position == CK_INVALID_OFFSET ) goto error;
        
    // add is_axis_motion()
    func = make_new_mfun( "int", "is_axis_motion", HidMsg_is_axis_motion ); // deprecated
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add isAxisMotion()
    func = make_new_mfun( "int", "isAxisMotion", HidMsg_is_axis_motion );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add is_button_down()
    func = make_new_mfun( "int", "is_button_down", HidMsg_is_button_down ); // deprecated
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add isButtonDown()
    func = make_new_mfun( "int", "isButtonDown", HidMsg_is_button_down );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add is_button_up()
    func = make_new_mfun( "int", "is_button_up", HidMsg_is_button_up ); // deprecated
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add isButtonUp()
    func = make_new_mfun( "int", "isButtonUp", HidMsg_is_button_up );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add is_mouse_motion()
    func = make_new_mfun( "int", "is_mouse_motion", HidMsg_is_mouse_motion ); // deprecated
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add isMouseMotion()
    func = make_new_mfun( "int", "isMouseMotion", HidMsg_is_mouse_motion );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add is_hat_motion()
    func = make_new_mfun( "int", "is_hat_motion", HidMsg_is_hat_motion ); // deprecated
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add isHatMotion()
    func = make_new_mfun( "int", "isHatMotion", HidMsg_is_hat_motion );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add isWheelMotion()
    func = make_new_mfun( "int", "isWheelMotion", HidMsg_is_wheel_motion );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // end the class import
    type_engine_import_class_end( env );


    // register deprecate
    type_engine_register_deprecate( env, "HidIn", "Hid" );

    // init base class Hid (copy of HidIn + constants)
    if( !type_engine_import_class_begin( env, "Hid", "Event",
                                         env->global(), HidIn_ctor, HidIn_dtor ) )
        return FALSE;
    
    // add open()
    func = make_new_mfun( "int", "open", HidIn_open );
    func->add_arg( "int", "type" );
    func->add_arg( "int", "num" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add open()
    func = make_new_mfun( "int", "open", HidIn_open_named );
    func->add_arg( "string", "name" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add openJoystick()
    func = make_new_mfun( "int", "openJoystick", HidIn_open_joystick );
    func->add_arg( "int", "num" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add openMouse()
    func = make_new_mfun( "int", "openMouse", HidIn_open_mouse );
    func->add_arg( "int", "num" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add openKeyboard()
    func = make_new_mfun( "int", "openKeyboard", HidIn_open_keyboard );
    func->add_arg( "int", "num" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add openTiltSensor()
    func = make_new_mfun( "int", "openTiltSensor", HidIn_open_tiltsensor );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add good()
    func = make_new_mfun( "int", "good", HidIn_good );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add num()
    func = make_new_mfun( "int", "num", HidIn_num );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add name()
    func = make_new_mfun( "string", "name", HidIn_name );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add printerr()
    func = make_new_mfun( "void", "printerr", HidIn_printerr );
    func->add_arg( "int", "print_or_not" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add recv()
    func = make_new_mfun( "int", "recv", HidIn_recv );
    func->add_arg( "HidMsg", "msg" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add read()
    func = make_new_mfun( "int", "read", HidIn_read );
    func->add_arg( "int", "type" );
    func->add_arg( "int", "which" );
    func->add_arg( "HidMsg", "msg" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add send()
    func = make_new_mfun( "int", "send", HidIn_send );
    func->add_arg( "HidMsg", "msg" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add can_wait()
    func = make_new_mfun( "int", "can_wait", HidIn_can_wait );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add readTiltSensor()
    func = make_new_sfun( "int[]", "readTiltSensor", HidIn_read_tilt_sensor );
    if( !type_engine_import_sfun( env, func ) ) goto error;
    
    // add globalTiltPollRate()
    func = make_new_sfun( "dur", "globalTiltPollRate", HidIn_ctrl_tiltPollRate );
    func->add_arg( "dur", "d" );
    if( !type_engine_import_sfun( env, func ) ) goto error;
    
    // add globalTiltPollRate()
    func = make_new_sfun( "dur", "globalTiltPollRate", HidIn_cget_tiltPollRate );
    if( !type_engine_import_sfun( env, func ) ) goto error;

    // add startCursorTrack()
    func = make_new_sfun( "int", "startCursorTrack", HidIn_start_cursor_track );
    if( !type_engine_import_sfun( env, func ) ) goto error;
    
    // add stopCursorTrack()
    func = make_new_sfun( "int", "stopCursorTrack", HidIn_stop_cursor_track );
    if( !type_engine_import_sfun( env, func ) ) goto error;
    
    // add member variable
    HidIn_offset_data = type_engine_import_mvar( env, "int", "@Hid_data", FALSE );
    if( HidIn_offset_data == CK_INVALID_OFFSET ) goto error;

    // add static member variable joystick
    if( type_engine_import_svar( env, "int", "JOYSTICK", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEV_JOYSTICK ) == FALSE )
        goto error;
    
    // add static member variable keyboard
    if( type_engine_import_svar( env, "int", "KEYBOARD", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEV_KEYBOARD ) == FALSE )
        goto error;
    
    // add static member variable mouse
    if( type_engine_import_svar( env, "int", "MOUSE", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEV_MOUSE ) == FALSE )
        goto error;
    
    // add static member variable wii_remote
    if( type_engine_import_svar( env, "int", "WII_REMOTE", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEV_WIIREMOTE ) == FALSE )
        goto error;
    
    // add static member variable wii_remote
    if( type_engine_import_svar( env, "int", "TILT_SENSOR", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEV_TILTSENSOR ) == FALSE )
        goto error;
    
    // add static member variable tablet
    if( type_engine_import_svar( env, "int", "TABLET", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEV_TABLET ) == FALSE )
        goto error;
    
    // add static member variable axisMotion
    if( type_engine_import_svar( env, "int", "AXIS_MOTION", TRUE,
                                 ( t_CKUINT ) &CK_HID_JOYSTICK_AXIS ) == FALSE )
        goto error;
    
    // add static member variable buttonDown
    if( type_engine_import_svar( env, "int", "BUTTON_DOWN", TRUE,
                                 ( t_CKUINT ) &CK_HID_BUTTON_DOWN ) == FALSE )
        goto error;
    
    // add static member variable buttonUp
    if( type_engine_import_svar( env, "int", "BUTTON_UP", TRUE,
                                 ( t_CKUINT ) &CK_HID_BUTTON_UP ) == FALSE )
        goto error;
    
    // add static member variable joystickHat
    if( type_engine_import_svar( env, "int", "JOYSTICK_HAT", TRUE,
                                 ( t_CKUINT ) &CK_HID_JOYSTICK_HAT ) == FALSE )
        goto error;
    
    // add static member variable JOYSTICK_BALL
    if( type_engine_import_svar( env, "int", "JOYSTICK_BALL", TRUE,
                                 ( t_CKUINT ) &CK_HID_JOYSTICK_BALL ) == FALSE )
        goto error;
    
    // add static member variable mouseMotion
    if( type_engine_import_svar( env, "int", "MOUSE_MOTION", TRUE,
                                 ( t_CKUINT ) &CK_HID_MOUSE_MOTION ) == FALSE )
        goto error;
    
    // add static member variable mouseWheel
    if( type_engine_import_svar( env, "int", "MOUSE_WHEEL", TRUE,
                                 ( t_CKUINT ) &CK_HID_MOUSE_WHEEL ) == FALSE )
        goto error;
    
    // add static member variable DEVICE_CONNECTED
    if( type_engine_import_svar( env, "int", "DEVICE_CONNECTED", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEVICE_CONNECTED ) == FALSE )
        goto error;
    
    // add static member variable DEVICE_DISCONNECTED
    if( type_engine_import_svar( env, "int", "DEVICE_DISCONNECTED", TRUE,
                                 ( t_CKUINT ) &CK_HID_DEVICE_DISCONNECTED ) == FALSE )
        goto error;
    
    // add static member variable ACCELEROMETER
    if( type_engine_import_svar( env, "int", "ACCELEROMETER", TRUE,
                                 ( t_CKUINT ) &CK_HID_ACCELEROMETER ) == FALSE )
        goto error;
    
    // add static member variable LED
    if( type_engine_import_svar( env, "int", "LED", TRUE,
                                 ( t_CKUINT ) &CK_HID_LED ) == FALSE )
        goto error;
    
    // add static member variable LED
    if( type_engine_import_svar( env, "int", "FORCE_FEEDBACK", TRUE,
                                 ( t_CKUINT ) &CK_HID_FORCE_FEEDBACK ) == FALSE )
        goto error;
    
    // end the class import
    type_engine_import_class_end( env );
    
    /*
    // init base class
    if( !type_engine_import_class_begin( env, "HidOut", "Object",
                                         env->global(), HidOut_ctor ) )
        return FALSE;

    // add open()
    func = make_new_mfun( "int", "open", HidOut_open );
    func->add_arg( "int", "port" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add good()
    func = make_new_mfun( "int", "good", HidOut_good );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add num()
    func = make_new_mfun( "int", "num", HidOut_num );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add name()
    func = make_new_mfun( "string", "name", HidOut_name );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add printerr()
    func = make_new_mfun( "void", "printerr", HidOut_printerr );
    func->add_arg( "int", "print_or_not" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add send()
    func = make_new_mfun( "int", "send", HidOut_send );
    func->add_arg( "HidMsg", "msg" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add member variable
    HidOut_offset_data = type_engine_import_mvar( env, "int", "@HidOut_data", FALSE );
    if( HidOut_offset_data == CK_INVALID_OFFSET ) goto error;

    // end the class import
    type_engine_import_class_end( env );
    */
    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}




#ifndef __DISABLE_MIDI__

// static
static t_CKUINT MidiRW_offset_data = 0;
static t_CKUINT MidiMsgOut_offset_data = 0;
static t_CKUINT MidiMsgIn_offset_data = 0;
//-----------------------------------------------------------------------------
// name: init_class_MidiRW()
// desc: ...
//-----------------------------------------------------------------------------
t_CKBOOL init_class_MidiRW( Chuck_Env * env )
{
    Chuck_DL_Func * func = NULL;

    // init base class
    if( !type_engine_import_class_begin( env, "MidiRW", "Object",
                                         env->global(), MidiRW_ctor, MidiRW_dtor ) )
        return FALSE;

    // add open()
    func = make_new_mfun( "int", "open", MidiRW_open );
    func->add_arg( "string", "filename" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add close()
    func = make_new_mfun( "int", "close", MidiRW_close );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add write()
    func = make_new_mfun( "int", "write", MidiRW_write );
    func->add_arg( "MidiMsg", "msg" );
    func->add_arg( "time", "t" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add read()
    func = make_new_mfun( "int", "read", MidiRW_read );
    func->add_arg( "MidiMsg", "msg" );
    //func->add_arg( "time", "t" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add member variable
    MidiRW_offset_data = type_engine_import_mvar( env, "int", "@MidiRW_data", FALSE );
    if( MidiRW_offset_data == CK_INVALID_OFFSET ) goto error;

    // end the class import
    type_engine_import_class_end( env );
    
        // init base class
    if( !type_engine_import_class_begin( env, "MidiMsgOut", "Object",
                                         env->global(), MidiMsgOut_ctor, MidiMsgOut_dtor ) )
        return FALSE;

    // add open()
    func = make_new_mfun( "int", "open", MidiMsgOut_open );
    func->add_arg( "string", "filename" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add close()
    func = make_new_mfun( "int", "close", MidiMsgOut_close );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add write()
    func = make_new_mfun( "int", "write", MidiMsgOut_write );
    func->add_arg( "MidiMsg", "msg" );
    func->add_arg( "time", "t" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add member variable
    MidiMsgOut_offset_data = type_engine_import_mvar( env, "int", "@MidiMsgOut_data", FALSE );
    if( MidiMsgOut_offset_data == CK_INVALID_OFFSET ) goto error;

    // end the class import
    type_engine_import_class_end( env );

        // init base class
    if( !type_engine_import_class_begin( env, "MidiMsgIn", "Object",
                                         env->global(), MidiMsgIn_ctor, MidiMsgIn_dtor ) )
        return FALSE;

    // add open()
    func = make_new_mfun( "int", "open", MidiMsgIn_open );
    func->add_arg( "string", "filename" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add close()
    func = make_new_mfun( "int", "close", MidiMsgIn_close );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add read()
    func = make_new_mfun( "int", "read", MidiMsgIn_read );
    func->add_arg( "MidiMsg", "msg" );
    //func->add_arg( "time", "t" );
    if( !type_engine_import_mfun( env, func ) ) goto error;

    // add member variable
    MidiMsgIn_offset_data = type_engine_import_mvar( env, "int", "@MidiMsgIn_data", FALSE );
    if( MidiMsgIn_offset_data == CK_INVALID_OFFSET ) goto error;

    // end the class import
    type_engine_import_class_end( env );

    // initialize
    // HidInManager::init();

    return TRUE;

error:

    // end the class import
    type_engine_import_class_end( env );
    
    return FALSE;
}



#endif // __DISABLE_MIDI__




// Object ctor
CK_DLL_CTOR( object_ctor )
{
    // log
    // EM_log( CK_LOG_FINEST, "Object constructor..." );

    // initialize
    OBJ_MEMBER_UINT(SELF, Object_offset_string) = 0;
}


// Object dtor
CK_DLL_DTOR( object_dtor )
{
    // log
    // EM_log( CK_LOG_FINEST, "Object destructor..." );

    // get the string
    Chuck_String * str = (Chuck_String *)OBJ_MEMBER_UINT(SELF, Object_offset_string);
    // release
    SAFE_RELEASE( str );
}


// toString
CK_DLL_MFUN( object_toString )
{
    // get the string
    Chuck_String * str = (Chuck_String *)OBJ_MEMBER_UINT(SELF, Object_offset_string);
    // allocate
    if( !str )
    {
        // new it
        str = (Chuck_String *)instantiate_and_initialize_object( &t_string, SHRED );
        // check it
        if( !str )
        {
            // TODO: make this exception
            fprintf( stderr, "[chuck]: Object.toString() out of memory!\n" );
            RETURN->v_object = NULL;
            return;
        }
        // set it
        ostringstream strout( ostringstream::out );
        // get the type
        Chuck_Type * type = SELF->type_ref;
        // write
        strout.setf( ios::hex, ios::basefield );
        strout << ((type != NULL) ? type->c_name() : "[VOID]") << ":" << (t_CKUINT)SELF;
        strout.flush();

        // done
        str->str = strout.str();
    }

    // set return
    RETURN->v_object = str;
}


// ctor
CK_DLL_CTOR( ugen_ctor )
{
}

CK_DLL_DTOR( ugen_dtor )
{
    // get as ugen
    // Chuck_UGen * ugen = (Chuck_UGen *)SELF;
}

CK_DLL_MFUN( ugen_op )
{
    // get as ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // get arg
    t_CKINT op = GET_CK_INT( ARGS );
    // set op
    ugen->m_op = op;
    // set return
    RETURN->v_int = ugen->m_op;

    // for multiple channels
    Chuck_DL_Return ret;
    for( t_CKUINT i = 0; i < ugen->m_multi_chan_size; i++ )
        ugen_op( ugen->m_multi_chan[i], ARGS, &ret, SHRED );
}

CK_DLL_MFUN( ugen_cget_op )
{
    // get as ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // set return
    RETURN->v_int = ugen->m_op;
}

CK_DLL_MFUN( ugen_last )
{
    // get as ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // set return
    RETURN->v_float = (t_CKFLOAT)ugen->m_last;
}

CK_DLL_MFUN( ugen_next )
{
    // get as ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // get arg
    t_CKFLOAT next = GET_CK_FLOAT( ARGS );
    // set op
    ugen->m_next = (SAMPLE)next;
    ugen->m_use_next = TRUE;
    // set return
    RETURN->v_float = (t_CKFLOAT)ugen->m_next;

    // for multiple channels
    Chuck_DL_Return ret;
    for( t_CKUINT i = 0; i < ugen->m_multi_chan_size; i++ )
        ugen_next( ugen->m_multi_chan[i], ARGS, &ret, SHRED );
}

CK_DLL_MFUN( ugen_cget_next )
{
    // get as ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // set return
    RETURN->v_float = ugen->m_use_next ? (t_CKFLOAT)ugen->m_next : 0.0f;
}

CK_DLL_MFUN( ugen_gain )
{
    // get as ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // get arg
    t_CKFLOAT gain = GET_CK_FLOAT( ARGS );
    // set op
    ugen->m_gain = (SAMPLE)gain;
    // set return
    RETURN->v_float = (t_CKFLOAT)ugen->m_gain;

    // for multiple channels
    Chuck_DL_Return ret;
    for( t_CKUINT i = 0; i < ugen->m_multi_chan_size; i++ )
        ugen_gain( ugen->m_multi_chan[i], ARGS, &ret, SHRED );
}

CK_DLL_MFUN( ugen_cget_gain )
{
    // get as ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // set return
    RETURN->v_float = (t_CKFLOAT)ugen->m_gain;
}

CK_DLL_CTRL( ugen_numChannels )
{
    // get ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // error
    EM_error3( "setting .numChannels is not yet supported (use the command line)..." );
}

CK_DLL_CGET( ugen_cget_numChannels )
{
    // get ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    // return
    RETURN->v_int = ugen->m_multi_chan_size == 0 ? 1 : ugen->m_multi_chan_size;
}

CK_DLL_CTRL( ugen_chan )
{
    // get ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    t_CKINT num = GET_NEXT_INT(ARGS);
    // check
    if( !ugen->m_multi_chan_size && num == 0 )
        RETURN->v_object = ugen;
    else if( num >= 0 && num < ugen->m_multi_chan_size )
        RETURN->v_object = ugen->m_multi_chan[num];
    else
        RETURN->v_object = NULL;
}

CK_DLL_CTRL( ugen_connected )
{
    // get ugen
    Chuck_UGen * ugen = (Chuck_UGen *)SELF;
    Chuck_UGen * right = (Chuck_UGen *)GET_NEXT_OBJECT(ARGS);

    // sanity
    t_CKINT ret = FALSE;
    if( !right )
    {
        ret = FALSE;
    }
    else
    {
        ret = right->is_connected_from( ugen );
    }

    RETURN->v_int = ret;
}


// ctor
CK_DLL_CTOR( uana_ctor )
{
    // make an actual blob
    Chuck_Object * blob = instantiate_and_initialize_object( &t_uanablob, SHRED );
    // TODO: check out of memory
    assert( blob != NULL );
    // make a blob proxy
    Chuck_UAnaBlobProxy * proxy = new Chuck_UAnaBlobProxy( blob );
    // remember it
    OBJ_MEMBER_INT(SELF, uana_offset_blob) = (t_CKINT)proxy;
    // HACK: DANGER: manually call blob's ctor
    uanablob_ctor( blob, NULL, NULL );
}

CK_DLL_DTOR( uana_dtor )
{
    // TODO: GC should release the blob!
}

CK_DLL_MFUN( uana_upchuck )
{
    // get as uana
    Chuck_UAna * uana = (Chuck_UAna *)SELF;
    // get shred
    Chuck_VM_Shred * derhs = SHRED;
    // make sure it's not NULL
    if( !derhs )
    {
        EM_error3( "UAna.upchuck() encountered NULL shred; operation aborting!" );
        return;
    }
    
    // get VM
    Chuck_VM * vm = derhs->vm_ref;
    // ensure
    if( !vm )
    {
        EM_error3( "UAna.upchuck() encountered NULL VM ref; operation aborting!" );
        return;
    }

    // check if time
    if( uana->m_uana_time < vm->shreduler()->now_system )
    {
        // for multiple channels
        Chuck_DL_Return ret;
        for( t_CKUINT i = 0; i < uana->m_multi_chan_size; i++ )
            uana_upchuck( uana->m_multi_chan[i], ARGS, &ret, SHRED );

        // tock it (TODO: order relative to multiple channels?)
        uana->system_tock( vm->shreduler()->now_system );
    }

    // return
    RETURN->v_object = uana->blobProxy()->realblob();
}

/* CK_DLL_MFUN( uana_blob )
{
    // get as uana
    Chuck_UAna * uana = (Chuck_UAna *)SELF;
    
    // TODO: return
    RETURN->v_object = NULL;
} */

CK_DLL_MFUN( uana_fvals )
{
    // get the fvals array
    Chuck_UAnaBlobProxy * blob = (Chuck_UAnaBlobProxy *)OBJ_MEMBER_INT(SELF, uana_offset_blob);
    RETURN->v_object = &blob->fvals();
}

CK_DLL_MFUN( uana_cvals )
{
    // get the fvals array
    Chuck_UAnaBlobProxy * blob = (Chuck_UAnaBlobProxy *)OBJ_MEMBER_INT(SELF, uana_offset_blob);
    RETURN->v_object = &blob->cvals();
}

CK_DLL_MFUN( uana_fval )
{
    // get index
    t_CKINT i = GET_NEXT_INT(ARGS);
    // get the fvals array
    Chuck_UAnaBlobProxy * blob = (Chuck_UAnaBlobProxy *)OBJ_MEMBER_INT(SELF, uana_offset_blob);
    Chuck_Array8 & fvals = blob->fvals();
    // check caps
    if( i < 0 || fvals.size() <= i ) RETURN->v_float = 0;
    else
    {
        // get
        t_CKFLOAT val;
        fvals.get( i, &val );
        RETURN->v_float = val;
    }
}

CK_DLL_MFUN( uana_cval )
{
    // get index
    t_CKINT i = GET_NEXT_INT(ARGS);
    // get the fvals array
    Chuck_UAnaBlobProxy * blob = (Chuck_UAnaBlobProxy *)OBJ_MEMBER_INT(SELF, uana_offset_blob);
    Chuck_Array16 & cvals = blob->cvals();
    // check caps
    if( i < 0 || cvals.size() <= i ) RETURN->v_complex.re = RETURN->v_complex.im = 0;
    else
    {
        // get
        t_CKCOMPLEX val;
        cvals.get( i, &val );
        RETURN->v_complex = val;
    }
}

CK_DLL_MFUN( uana_connected )
{
    // get ugen
    Chuck_UAna * uana = (Chuck_UAna *)SELF;
    Chuck_UAna * right = (Chuck_UAna *)GET_NEXT_OBJECT(ARGS);

    // sanity
    t_CKINT ret = FALSE;
    if( !right )
    {
        ret = FALSE;
    }
    else
    {
        ret = right->is_up_connected_from( uana );
    }

    RETURN->v_int = ret;
}


// blob proxy implementation
Chuck_UAnaBlobProxy::Chuck_UAnaBlobProxy( Chuck_Object * blob )
{
    m_blob = blob;
    assert( m_blob != NULL );
    // add reference
    m_blob->add_ref();
}

Chuck_UAnaBlobProxy::~Chuck_UAnaBlobProxy()
{
    // release
    SAFE_RELEASE( m_blob );
}

t_CKTIME & Chuck_UAnaBlobProxy::when()
{
    // TODO: DANGER: is this actually returning correct reference?!
    return OBJ_MEMBER_TIME(m_blob, uanablob_offset_when);
}

Chuck_Array8 & Chuck_UAnaBlobProxy::fvals()
{
    // TODO: DANGER: is this actually returning correct reference?!
    Chuck_Array8 * arr8 = (Chuck_Array8 *)OBJ_MEMBER_INT(m_blob, uanablob_offset_fvals);
    assert( arr8 != NULL );
    return *arr8;
}

Chuck_Array16 & Chuck_UAnaBlobProxy::cvals()
{
    // TODO: DANGER: is this actually returning correct reference?!
    Chuck_Array16 * arr16 = (Chuck_Array16 *)OBJ_MEMBER_INT(m_blob, uanablob_offset_cvals);
    assert( arr16 != NULL );
    return *arr16;
}

// get proxy
Chuck_UAnaBlobProxy * getBlobProxy( const Chuck_UAna * uana )
{
    return (Chuck_UAnaBlobProxy *)OBJ_MEMBER_INT(uana, uana_offset_blob);
}

// ctor
CK_DLL_CTOR( uanablob_ctor )
{
    // when
    OBJ_MEMBER_TIME(SELF, uanablob_offset_when) = 0;
    // fvals
    Chuck_Array8 * arr8 = new Chuck_Array8( 8 );
    initialize_object( arr8, &t_array );
    // TODO: check out of memory
    arr8->add_ref();
    OBJ_MEMBER_INT(SELF, uanablob_offset_fvals) = (t_CKINT)arr8;
    // cvals
    Chuck_Array16 * arr16 = new Chuck_Array16( 8 );
    initialize_object( arr16, &t_array );
    // TODO: check out of memory
    arr16->add_ref();
    OBJ_MEMBER_INT(SELF, uanablob_offset_cvals) = (t_CKINT)arr16;
}

// dtor
CK_DLL_DTOR( uanablob_dtor )
{
    // get array
    Chuck_Array8 * arr8 = (Chuck_Array8 *)OBJ_MEMBER_INT(SELF, uanablob_offset_fvals);
    // release it
    arr8->release();
    OBJ_MEMBER_INT(SELF, uanablob_offset_fvals) = 0;
    
    // get array
    Chuck_Array16 * arr16 = (Chuck_Array16 *)OBJ_MEMBER_INT(SELF, uanablob_offset_cvals);
    // release it
    arr16->release();
    OBJ_MEMBER_INT(SELF, uanablob_offset_cvals) = 0;
    
    OBJ_MEMBER_TIME(SELF, uanablob_offset_when) = 0;
}

CK_DLL_MFUN( uanablob_when )
{
    // set return
    RETURN->v_time = OBJ_MEMBER_TIME(SELF, uanablob_offset_when);
}

CK_DLL_MFUN( uanablob_fvals )
{
    // set return
    RETURN->v_object = (Chuck_Array8 *)OBJ_MEMBER_INT(SELF, uanablob_offset_fvals);
}

CK_DLL_MFUN( uanablob_fval )
{
    // get index
    t_CKINT i = GET_NEXT_INT(ARGS);
    // get the fvals array
    Chuck_Array8 * fvals = (Chuck_Array8 *)OBJ_MEMBER_INT(SELF, uanablob_offset_fvals);
    // check caps
    if( i < 0 || fvals->size() <= i ) RETURN->v_float = 0;
    else
    {
        // get
        t_CKFLOAT val;
        fvals->get( i, &val );
        RETURN->v_float = val;
    }
}

CK_DLL_MFUN( uanablob_cval )
{
    // get index
    t_CKINT i = GET_NEXT_INT(ARGS);
    // get the fvals array
    Chuck_Array16 * cvals = (Chuck_Array16 *)OBJ_MEMBER_INT(SELF, uanablob_offset_cvals);
    // check caps
    if( i < 0 || cvals->size() <= i ) RETURN->v_complex.re = RETURN->v_complex.im = 0;
    else
    {
        // get
        t_CKCOMPLEX val;
        cvals->get( i, &val );
        RETURN->v_complex = val;
    }
}

CK_DLL_MFUN( uanablob_cvals )
{
    // set return
    RETURN->v_object = (Chuck_Array16 *)OBJ_MEMBER_INT(SELF, uanablob_offset_cvals);
}

// ctor
CK_DLL_CTOR( event_ctor )
{
//  OBJ_MEMBER_INT(SELF, event_offset_data) = (t_CKUINT)new Data_Event;
//  Chuck_Event * event = (Chuck_Event *)SELF;
}


CK_DLL_DTOR( event_dtor )
{
//  delete (Data_Event *)OBJ_MEMBER_INT(SELF, event_offset_data);
}

CK_DLL_MFUN( event_signal )
{
    Chuck_Event * d = (Chuck_Event *)SELF;
    d->signal();
}

CK_DLL_MFUN( event_broadcast )
{
    Chuck_Event * d = (Chuck_Event *)SELF;
    d->broadcast();
}

CK_DLL_MFUN( event_wait )
{
    // Chuck_Event * d = (Chuck_Event *)SELF;
    assert( FALSE );
}

CK_DLL_MFUN( event_can_wait )
{
    RETURN->v_int = TRUE;
}


//-----------------------------------------------------------------------------
// IO API
//-----------------------------------------------------------------------------
CK_DLL_MFUN( io_dummy )
{
    EM_error3( "(IO): internal error! inside an abstract function! help!" );
    EM_error3( "    note: please hunt down someone (e.g., Ge) to fix this..." );
    assert( FALSE );
    RETURN->v_int = 0;
}

CK_DLL_SFUN( io_newline )
{
    RETURN->v_string = g_newline;
}


//-----------------------------------------------------------------------------
// FileIO API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( fileio_ctor )
{ } 

CK_DLL_DTOR( fileio_dtor )
{ }

CK_DLL_MFUN( fileio_open )
{
    std::string filename = GET_NEXT_STRING(ARGS)->str;    
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    t_CKINT default_flags =
        Chuck_IO_File::FLAG_READ_WRITE | Chuck_IO_File::TYPE_ASCII;

    RETURN->v_int = f->open(filename, default_flags);
}

CK_DLL_MFUN( fileio_openflags )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    std::string filename = GET_NEXT_STRING(ARGS)->str;
    t_CKINT flags = GET_NEXT_INT(ARGS);

    RETURN->v_int = f->open(filename, flags);
}

CK_DLL_MFUN( fileio_close )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    f->close();
}

CK_DLL_MFUN( fileio_good )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    RETURN->v_int = f->good();
}

CK_DLL_MFUN( fileio_flush )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    f->flush();
}

CK_DLL_MFUN( fileio_getmode )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    RETURN->v_int = f->mode();
}

CK_DLL_MFUN( fileio_setmode )
{
    t_CKINT flag = GET_NEXT_INT(ARGS);
    
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    f->mode( flag );
    RETURN->v_int = 0;
}

CK_DLL_MFUN( fileio_size )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    RETURN->v_int = f->size();
}

CK_DLL_MFUN( fileio_seek )
{
    t_CKINT pos = GET_NEXT_INT(ARGS);
    
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    f->seek(pos);
}

CK_DLL_MFUN( fileio_tell )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    RETURN->v_int = f->tell();
}

CK_DLL_MFUN( fileio_isdir )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    RETURN->v_int = f->isDir();
}

CK_DLL_MFUN( fileio_dirlist )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    Chuck_Array4 * a = f->dirList();
    RETURN->v_object = a;
}

/*
CK_DLL_MFUN( fileio_read )
{
    t_CKINT len = GET_NEXT_INT(ARGS);
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    
    Chuck_String * s = f->read( len );
    RETURN->v_object = s;
}
*/

CK_DLL_MFUN( fileio_readline )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    Chuck_String * ret = f->readLine();
    RETURN->v_object = ret;
}

CK_DLL_MFUN( fileio_readint )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    t_CKINT defaultflags = Chuck_IO::READ_INT32;
    
    /* (ATODO: doesn't look like asynchronous reading will work)
     if (f->mode() == Chuck_IO::MODE_ASYNC)
     {
     // set up arguments
     Chuck_IO::async_args *args = new Chuck_IO::async_args;
     args->RETURN = (void *)RETURN;
     args->fileio_obj = f;
     args->intArg = defaultflags;
     // set shred to wait for I/O completion
     f->m_asyncEvent->wait( SHRED, g_vm );
     // start thread
     bool ret = f->m_thread->start( f->readInt_thread, (void *)args );
     if (!ret) {
     cerr << "m_thread->start failed; recreating m_thread" << endl;
     delete f->m_thread;
     f->m_thread = new XThread;
     ret = f->m_thread->start( f->readInt_thread, (void *)args );
     if (!ret) {
     EM_error3( "(FileIO): failed to start thread for asynchronous mode I/O" );
     }
     }
     } else {*/
    t_CKINT ret = f->readInt( defaultflags );
    RETURN->v_int = ret;
    //}
    // ATODO: Debug
    //sleep(1);
    //cerr << "fileio_readint exiting" << endl;
}

CK_DLL_MFUN( fileio_readintflags )
{    
    t_CKINT flags = GET_NEXT_INT(ARGS);
    
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    t_CKINT ret = f->readInt( flags );
    
    RETURN->v_int = ret;
}

CK_DLL_MFUN( fileio_readfloat )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    t_CKFLOAT ret = f->readFloat();
    RETURN->v_float = ret;
}

CK_DLL_MFUN( fileio_eof )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    t_CKBOOL ret = f->eof();
    RETURN->v_int = ret;
}

CK_DLL_MFUN( fileio_more )
{
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    t_CKBOOL ret = !f->eof();
    RETURN->v_int = ret;
}

CK_DLL_MFUN( fileio_writestring )
{
    std::string val = GET_NEXT_STRING(ARGS)->str;
    
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    if (f->mode() == Chuck_IO::MODE_ASYNC)
    {
        // set up arguments
        Chuck_IO::async_args *args = new Chuck_IO::async_args;
        args->RETURN = (void *)RETURN;
        args->fileio_obj = f;
        args->stringArg = std::string(val);
        // set shred to wait for I/O completion
        // TODO: make sure using g_vm is OK
        f->m_asyncEvent->wait( SHRED, g_vm );
        // start thread
        bool ret = f->m_thread->start( f->writeStr_thread, (void *)args );
        if (!ret) {
            // for some reason, the XThread object needs to be
            // deleted and reconstructed every time after call #375
            delete f->m_thread;
            f->m_thread = new XThread;
            ret = f->m_thread->start( f->writeStr_thread, (void *)args );
            if (!ret) {
                EM_error3( "(FileIO): failed to start thread for asynchronous mode I/O" );
            }
        }
    } else {
        f->write(val);
    }
}

CK_DLL_MFUN( fileio_writeint )
{
    t_CKINT val = GET_NEXT_INT(ARGS);
    
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    if (f->mode() == Chuck_IO::MODE_ASYNC)
    {
        // set up arguments
        Chuck_IO::async_args *args = new Chuck_IO::async_args;
        args->RETURN = (void *)RETURN;
        args->fileio_obj = f;
        args->intArg = val;
        // set shred to wait for I/O completion
        // TODO: make sure using g_vm is OK
        f->m_asyncEvent->wait( SHRED, g_vm );
        // start thread
        bool ret = f->m_thread->start( f->writeInt_thread, (void *)args );
        if (!ret) {
            // for some reason, the XThread object needs to be
            // deleted and reconstructed every time after call #375
            delete f->m_thread;
            f->m_thread = new XThread;
            ret = f->m_thread->start( f->writeInt_thread, (void *)args );
            if (!ret) {
                EM_error3( "(FileIO): failed to start thread for asynchronous mode I/O" );
            }
        }
    } else {
        f->write(val);
    }
}

CK_DLL_MFUN( fileio_writefloat )
{
    t_CKFLOAT val = GET_NEXT_FLOAT(ARGS);
    
    Chuck_IO_File * f = (Chuck_IO_File *)SELF;
    if (f->mode() == Chuck_IO::MODE_ASYNC)
    {
        // set up arguments
        Chuck_IO::async_args *args = new Chuck_IO::async_args;
        args->RETURN = (void *)RETURN;
        args->fileio_obj = f;
        args->floatArg = val;
        // set shred to wait for I/O completion
        // TODO: make sure using g_vm is OK
        f->m_asyncEvent->wait( SHRED, g_vm );
        // start thread
        bool ret = f->m_thread->start( f->writeFloat_thread, (void *)args );
        if (!ret) {
            // for some reason, the XThread object needs to be
            // deleted and reconstructed every time after call #375
            delete f->m_thread;
            f->m_thread = new XThread;
            ret = f->m_thread->start( f->writeFloat_thread, (void *)args );
            if (!ret) {
                EM_error3( "(FileIO): failed to start thread for asynchronous mode I/O" );
            }
        }
    } else {
        f->write(val);
    }
}




//-----------------------------------------------------------------------------
// Shred API
//-----------------------------------------------------------------------------

//CK_DLL_CTOR( shred_ctor );
//CK_DLL_DTOR( shred_dtor ); //

CK_DLL_MFUN( shred_exit )
{
    Chuck_VM_Shred * derhs = (Chuck_VM_Shred *)SELF;
    // end the shred
    derhs->is_done = TRUE;
    derhs->is_running = FALSE;
}

CK_DLL_MFUN( shred_running )
{
    Chuck_VM_Shred * derhs = (Chuck_VM_Shred *)SELF;
    RETURN->v_int = derhs->is_running;
}

CK_DLL_MFUN( shred_done )
{
    Chuck_VM_Shred * derhs = (Chuck_VM_Shred *)SELF;
    RETURN->v_int = derhs->is_done;
}

CK_DLL_MFUN( shred_clone )
{
}

CK_DLL_MFUN( shred_id )
{
    Chuck_VM_Shred * derhs = (Chuck_VM_Shred *)SELF;
    // return the id
    RETURN->v_int = derhs->xid;
}

CK_DLL_MFUN( shred_yield )
{
    Chuck_VM_Shred * derhs = (Chuck_VM_Shred *)SELF;
    Chuck_VM * vm = derhs->vm_ref;

    // suspend
    derhs->is_running = FALSE;
    // reshredule
    vm->shreduler()->shredule( derhs, derhs->now );
}

CK_DLL_MFUN( shred_numArgs )
{
    Chuck_VM_Shred * derhs = (Chuck_VM_Shred *)SELF;

    // get the number of arguments
    RETURN->v_int = derhs->args.size();
}

CK_DLL_MFUN( shred_getArg )
{
    Chuck_VM_Shred * derhs = (Chuck_VM_Shred *)SELF;

    // get index
    t_CKINT i = GET_NEXT_INT(ARGS);
    // total
    t_CKINT num = derhs->args.size();

    Chuck_String * str = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    str->str = ( i < num ? derhs->args[i] : "" );
    RETURN->v_string = str; 
}

CK_DLL_MFUN( string_length )
{
    Chuck_String * s = (Chuck_String *)SELF;
    RETURN->v_int = s->str.length();
}

CK_DLL_MFUN( string_upper )
{
    Chuck_String * s = (Chuck_String *)SELF;
    Chuck_String * str = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    str->str = toupper( s->str );
    RETURN->v_string = str;
}

CK_DLL_MFUN( string_lower )
{
    Chuck_String * s = (Chuck_String *)SELF;
    Chuck_String * str = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    str->str = tolower( s->str );
    RETURN->v_string = str;
}

CK_DLL_MFUN( string_ltrim )
{
    Chuck_String * s = (Chuck_String *)SELF;
    Chuck_String * str = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    str->str = ltrim( s->str );
    RETURN->v_string = str;
}

CK_DLL_MFUN( string_rtrim )
{
    Chuck_String * s = (Chuck_String *)SELF;
    Chuck_String * str = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    str->str = rtrim( s->str );
    RETURN->v_string = str;
}

CK_DLL_MFUN( string_trim )
{
    Chuck_String * s = (Chuck_String *)SELF;
    Chuck_String * str = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    str->str = trim( s->str );
    RETURN->v_string = str;
}

CK_DLL_MFUN( string_toString )
{
    Chuck_String * s = (Chuck_String *)SELF;
    RETURN->v_string = s;
}

/*
CK_DLL_MFUN( string_set_at )
{
    Chuck_String * s = (Chuck_String *)SELF;
    t_CKINT c = GET_CK_INT(
    RETURN->v_int = s->str.length();
}

CK_DLL_MFUN( string_get_at )
{
    Chuck_String * s = (Chuck_String *)SELF;
    RETURN->v_int = s->str.length();
}
*/


// array.size()
CK_DLL_MFUN( array_get_size )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    RETURN->v_int = array->size();
}

// array.size()
CK_DLL_MFUN( array_set_size )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    t_CKINT size = GET_NEXT_INT(ARGS);
    if( size < 0 )
    {
        // TODO: make this exception
        fprintf( stderr, "[chuck](via array): attempt to set negative array size!\n" );
        RETURN->v_int = 0;
    }
    else
    {
        array->set_size( size );
        RETURN->v_int = array->size();
    }
    RETURN->v_int = array->size();
}

// array.clear()
CK_DLL_MFUN( array_clear )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    array->clear();
}

// array.reset()
CK_DLL_MFUN( array_reset )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    array->clear();
    // default capacity
    array->set_capacity( 8 );
}

// array.cap()
CK_DLL_MFUN( array_set_capacity )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    t_CKINT capacity = GET_NEXT_INT(ARGS);
    if( capacity < 0 )
    {
        // TODO: make this exception
        fprintf( stderr, "[chuck](via array): attempt to set negative array capacity!\n" );
        RETURN->v_int = 0;
    }
    else
    {
        array->set_capacity( capacity );
        RETURN->v_int = array->capacity();
    }
}

// array.cap()
CK_DLL_MFUN( array_get_capacity_hack )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    RETURN->v_int = array->size();
}

// array.capacity()
CK_DLL_MFUN( array_get_capacity )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    RETURN->v_int = array->capacity();
}

// array.find()
CK_DLL_MFUN( array_find )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    Chuck_String * name = GET_NEXT_STRING( ARGS );
    RETURN->v_int = array->find( name->str );
}

// array.erase()
CK_DLL_MFUN( array_erase )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    Chuck_String * name = GET_NEXT_STRING( ARGS );
    RETURN->v_int = array->erase( name->str );
}

// array.push_back()
CK_DLL_MFUN( array_push_back )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    // TODO: make this safe!
    if( array->data_type_size() == CHUCK_ARRAY4_DATASIZE )
        RETURN->v_int = ((Chuck_Array4 *)array)->push_back( GET_NEXT_UINT( ARGS ) );
    else if( array->data_type_size() == CHUCK_ARRAY8_DATASIZE )
        RETURN->v_int = ((Chuck_Array8 *)array)->push_back( GET_NEXT_FLOAT( ARGS ) );
    else if( array->data_type_size() == CHUCK_ARRAY16_DATASIZE )
        RETURN->v_int = ((Chuck_Array16 *)array)->push_back( GET_NEXT_COMPLEX( ARGS ) );
    else
        assert( FALSE );
}

// array.pop_back()
CK_DLL_MFUN( array_pop_back )
{
    Chuck_Array * array = (Chuck_Array *)SELF;
    // TODO: make this safe!
    if( array->data_type_size() == CHUCK_ARRAY4_DATASIZE )
        RETURN->v_int = ((Chuck_Array4 *)array)->pop_back( );
    else if( array->data_type_size() == CHUCK_ARRAY8_DATASIZE )
        RETURN->v_int = ((Chuck_Array8 *)array)->pop_back( );
    else if( array->data_type_size() == CHUCK_ARRAY16_DATASIZE )
        RETURN->v_int = ((Chuck_Array16 *)array)->pop_back( );
    else
        assert( FALSE );
}


#ifndef __DISABLE_MIDI__

//-----------------------------------------------------------------------------
// MidiIn API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( MidiIn_ctor )
{
    MidiIn * min = new MidiIn;
    min->SELF = SELF;
    OBJ_MEMBER_INT(SELF, MidiIn_offset_data) = (t_CKINT)min;
}

CK_DLL_DTOR( MidiIn_dtor )
{
    delete (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    OBJ_MEMBER_INT(SELF, MidiIn_offset_data) = 0;
}

CK_DLL_MFUN( MidiIn_open )
{
    MidiIn * min = (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    t_CKINT port = GET_CK_INT(ARGS);
    RETURN->v_int = min->open( port );
}

CK_DLL_MFUN( MidiIn_good )
{
    MidiIn * min = (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    RETURN->v_int = (t_CKINT)min->good();
}

CK_DLL_MFUN( MidiIn_num )
{
    MidiIn * min = (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    RETURN->v_int = min->num();
}

CK_DLL_MFUN( MidiIn_printerr )
{
    MidiIn * min = (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    t_CKINT print_or_not = GET_CK_INT(ARGS);
    min->set_suppress( !print_or_not );
}

CK_DLL_MFUN( MidiIn_name )
{
    MidiIn * min = (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    // TODO: memory leak, please fix, Thanks.
    Chuck_String * a = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    // only if valid
    if( min->good() )
        a->str = min->min->getPortName( min->num() );
    RETURN->v_string = a;
}

CK_DLL_MFUN( MidiIn_recv )
{
    MidiIn * min = (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    Chuck_Object * fake_msg = GET_CK_OBJECT(ARGS);
    MidiMsg the_msg;
    RETURN->v_int = min->recv( &the_msg );
    if( RETURN->v_int )
    {
        OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data1) = the_msg.data[0];
        OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data2) = the_msg.data[1];
        OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data3) = the_msg.data[2];
    }
}


CK_DLL_MFUN( MidiIn_can_wait )
{
    MidiIn * min = (MidiIn *)OBJ_MEMBER_INT(SELF, MidiIn_offset_data);
    RETURN->v_int = min->empty();
}


//-----------------------------------------------------------------------------
// MidiOut API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( MidiOut_ctor )
{
    OBJ_MEMBER_INT(SELF, MidiOut_offset_data) = (t_CKUINT)new MidiOut;
}

CK_DLL_DTOR( MidiOut_dtor )
{
    delete (MidiOut *)OBJ_MEMBER_INT(SELF, MidiOut_offset_data);
    OBJ_MEMBER_INT(SELF, MidiOut_offset_data) = 0;
}

CK_DLL_MFUN( MidiOut_open )
{
    MidiOut * mout = (MidiOut *)OBJ_MEMBER_INT(SELF, MidiOut_offset_data);
    t_CKINT port = GET_CK_INT(ARGS);
    RETURN->v_int = mout->open( port );
}

CK_DLL_MFUN( MidiOut_good )
{
    MidiOut * mout = (MidiOut *)OBJ_MEMBER_INT(SELF, MidiOut_offset_data);
    RETURN->v_int = (t_CKINT)mout->good();
}

CK_DLL_MFUN( MidiOut_num )
{
    MidiOut * mout = (MidiOut *)OBJ_MEMBER_INT(SELF, MidiOut_offset_data);
    RETURN->v_int = mout->num();
}

CK_DLL_MFUN( MidiOut_name )
{
    MidiOut * mout = (MidiOut *)OBJ_MEMBER_INT(SELF, MidiOut_offset_data);
    // TODO: memory leak, please fix, Thanks.
    Chuck_String * a = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    // only if valid
    if( mout->good() )
        a->str = mout->mout->getPortName( mout->num() );
    RETURN->v_string = a;
}

CK_DLL_MFUN( MidiOut_printerr )
{
    MidiOut * mout = (MidiOut *)OBJ_MEMBER_INT(SELF, MidiOut_offset_data);
    t_CKINT print_or_not = GET_CK_INT(ARGS);
    mout->set_suppress( !print_or_not );
}

CK_DLL_MFUN( MidiOut_send )
{
    MidiOut * mout = (MidiOut *)OBJ_MEMBER_INT(SELF, MidiOut_offset_data);
    Chuck_Object * fake_msg = GET_CK_OBJECT(ARGS);
    MidiMsg the_msg;
    the_msg.data[0] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data1);
    the_msg.data[1] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data2);
    the_msg.data[2] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data3);
    RETURN->v_int = mout->send( &the_msg );
}

#endif // __DISABLE_MIDI__


//-----------------------------------------------------------------------------
// HidMsg API
//-----------------------------------------------------------------------------
CK_DLL_MFUN( HidMsg_is_axis_motion )
{
    RETURN->v_int = ( ( t_CKINT ) OBJ_MEMBER_INT( SELF, HidMsg_offset_type ) == 
                      CK_HID_JOYSTICK_AXIS ? 1 : 0 );
}

CK_DLL_MFUN( HidMsg_is_button_down )
{
    RETURN->v_int = ( ( t_CKINT ) OBJ_MEMBER_INT( SELF, HidMsg_offset_type ) == 
                      CK_HID_BUTTON_DOWN ? 1 : 0 );
}

CK_DLL_MFUN( HidMsg_is_button_up )
{
    RETURN->v_int = ( ( t_CKINT ) OBJ_MEMBER_INT( SELF, HidMsg_offset_type ) == 
                      CK_HID_BUTTON_UP ? 1 : 0 );
}

CK_DLL_MFUN( HidMsg_is_mouse_motion )
{
    RETURN->v_int = ( ( t_CKINT ) OBJ_MEMBER_INT( SELF, HidMsg_offset_type ) == 
                      CK_HID_MOUSE_MOTION ? 1 : 0 );
}

CK_DLL_MFUN( HidMsg_is_hat_motion )
{
    RETURN->v_int = ( ( t_CKINT ) OBJ_MEMBER_INT( SELF, HidMsg_offset_type ) == 
                      CK_HID_JOYSTICK_HAT ? 1 : 0 );
}

CK_DLL_MFUN( HidMsg_is_wheel_motion )
{
    RETURN->v_int = ( ( t_CKINT ) OBJ_MEMBER_INT( SELF, HidMsg_offset_type ) == 
                      CK_HID_MOUSE_WHEEL ? 1 : 0 );
}

//-----------------------------------------------------------------------------
// HidIn API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( HidIn_ctor )
{
    HidIn * min = new HidIn;
    min->SELF = SELF;
    OBJ_MEMBER_INT(SELF, HidIn_offset_data) = (t_CKINT)min;
}

CK_DLL_DTOR( HidIn_dtor )
{
    delete (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    OBJ_MEMBER_INT(SELF, HidIn_offset_data) = 0;
}

CK_DLL_MFUN( HidIn_open )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    t_CKINT type = GET_NEXT_INT(ARGS);
    t_CKINT num = GET_NEXT_INT(ARGS);
    RETURN->v_int = min->open( type, num );
}

CK_DLL_MFUN( HidIn_open_named )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    Chuck_String * name = GET_NEXT_STRING(ARGS);
    RETURN->v_int = min->open( name->str );
}

CK_DLL_MFUN( HidIn_open_joystick )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    t_CKINT num = GET_NEXT_INT(ARGS);
    RETURN->v_int = min->open( CK_HID_DEV_JOYSTICK, num );
}

CK_DLL_MFUN( HidIn_open_mouse )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    t_CKINT num = GET_NEXT_INT(ARGS);
    RETURN->v_int = min->open( CK_HID_DEV_MOUSE, num );
}

CK_DLL_MFUN( HidIn_open_keyboard )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    t_CKINT num = GET_NEXT_INT(ARGS);
    RETURN->v_int = min->open( CK_HID_DEV_KEYBOARD, num );
}

CK_DLL_MFUN( HidIn_open_tiltsensor )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    RETURN->v_int = min->open( CK_HID_DEV_TILTSENSOR, 0 );
}

CK_DLL_MFUN( HidIn_good )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    RETURN->v_int = (t_CKINT)min->good();
}

CK_DLL_MFUN( HidIn_num )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    RETURN->v_int = min->num();
}

CK_DLL_MFUN( HidIn_printerr )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    t_CKINT print_or_not = GET_CK_INT(ARGS);
    min->set_suppress( !print_or_not );
}

CK_DLL_MFUN( HidIn_name )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    // TODO: memory leak, please fix, Thanks.
    // Chuck_String * a = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    // only if valid
    // if( min->good() )
    //     a->str = min->phin->getPortName( min->num() );
    // TODO: is null problem?
    RETURN->v_string = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    RETURN->v_string->str = min->name();
}

CK_DLL_MFUN( HidIn_recv )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    Chuck_Object * fake_msg = GET_CK_OBJECT(ARGS);
    HidMsg the_msg;
    RETURN->v_int = min->recv( &the_msg );
    if( RETURN->v_int )
    {
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_device_type) = the_msg.device_type;
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_device_num) = the_msg.device_num;
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_type) = the_msg.type;
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_which) = the_msg.eid;
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_idata) = the_msg.idata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_fdata) = the_msg.fdata[0];
        
        // mouse motion specific member variables
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_deltax) = the_msg.idata[0];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_deltay) = the_msg.idata[1];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_cursorx) = the_msg.idata[2];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_cursory) = the_msg.idata[3];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_scaledcursorx) = the_msg.fdata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_scaledcursory) = the_msg.fdata[1];
        
        // axis motion specific member variables
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_axis_position) = the_msg.idata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_scaled_axis_position) = the_msg.fdata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_axis_position2) = the_msg.fdata[0];
        
        // hat motion specific variables
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_hat_position) = the_msg.idata[0];
        
        // keyboard specific variables
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_key) = the_msg.idata[1];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_ascii) = the_msg.idata[2];
        
        // accelerometer (tilt sensor, wii remote) specific members
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_x) = the_msg.idata[0];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_y) = the_msg.idata[1];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_z) = the_msg.idata[2];        
    }
}

CK_DLL_MFUN( HidIn_read )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    t_CKINT type = GET_NEXT_INT(ARGS);
    t_CKINT num = GET_NEXT_INT(ARGS);
    Chuck_Object * fake_msg = GET_NEXT_OBJECT(ARGS);
    
    HidMsg the_msg;
    
    RETURN->v_int = min->read( type, num, &the_msg );
    
    if( RETURN->v_int )
    {
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_device_type) = the_msg.device_type;
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_device_num) = the_msg.device_num;
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_type) = the_msg.type;
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_which) = the_msg.eid;
        
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_idata) = the_msg.idata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_fdata) = the_msg.fdata[0];
        
        // mouse motion specific member members
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_deltax) = the_msg.idata[0];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_deltay) = the_msg.idata[1];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_cursorx) = the_msg.idata[2];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_cursory) = the_msg.idata[3];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_scaledcursorx) = the_msg.fdata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_scaledcursory) = the_msg.fdata[1];
        
        // joystick axis specific member members
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_axis_position) = the_msg.idata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_scaled_axis_position) = the_msg.fdata[0];
        OBJ_MEMBER_FLOAT(fake_msg, HidMsg_offset_axis_position2) = the_msg.fdata[0];
        
        // joystick hat specific member members
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_hat_position) = the_msg.idata[0];
        
        // keyboard specific members
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_key) = the_msg.idata[1];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_ascii) = the_msg.idata[2];
        
        // accelerometer (tilt sensor, wii remote) specific members
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_x) = the_msg.idata[0];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_y) = the_msg.idata[1];
        OBJ_MEMBER_INT(fake_msg, HidMsg_offset_z) = the_msg.idata[2];
    }
}    

CK_DLL_MFUN( HidIn_send )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    Chuck_Object * fake_msg = GET_NEXT_OBJECT(ARGS);
    
    HidMsg the_msg;
    the_msg.device_type = OBJ_MEMBER_INT( fake_msg, HidMsg_offset_device_type );
    the_msg.device_num = OBJ_MEMBER_INT( fake_msg, HidMsg_offset_device_num );
    the_msg.type = OBJ_MEMBER_INT( fake_msg, HidMsg_offset_type );
    the_msg.eid = OBJ_MEMBER_INT( fake_msg, HidMsg_offset_which );
    the_msg.idata[0] = OBJ_MEMBER_INT( fake_msg, HidMsg_offset_idata );
    
    RETURN->v_int = min->send( &the_msg );
}

CK_DLL_MFUN( HidIn_can_wait )
{
    HidIn * min = (HidIn *)OBJ_MEMBER_INT(SELF, HidIn_offset_data);
    RETURN->v_int = min->empty();
}

CK_DLL_SFUN( HidIn_read_tilt_sensor )
{
    static HidIn * hi;
    static t_CKBOOL hi_good = TRUE;
    
    Chuck_Array4 * array = new Chuck_Array4( FALSE, 3 );
    array->set( 0, 0 );
    array->set( 1, 0 );
    array->set( 2, 0 );
    
    // TODO: reference count?
    array->add_ref();
    RETURN->v_object = array;
    
    if( hi_good == FALSE )
        return;
    
    if( !hi )
    {
        hi = new HidIn;
        if( !hi->open( CK_HID_DEV_TILTSENSOR, 0 ) )
        {
            hi_good = FALSE;
            return;
        }
    }

    HidMsg msg;
    
    if( !hi->read( CK_HID_ACCELEROMETER, 0, &msg ) )
    {
        return;
    }
    
    array->set( 0, msg.idata[0] );
    array->set( 1, msg.idata[1] );
    array->set( 2, msg.idata[2] );
}

CK_DLL_SFUN( HidIn_ctrl_tiltPollRate )
{
    // get srate
    if( !SHRED || !SHRED->vm_ref )
    {
        // problem
        fprintf( stderr, "[chuck](via HID): can't set tiltPollRate on NULL shred/VM...\n" );
        RETURN->v_dur = 0;
        return;
    }
    t_CKFLOAT srate = SHRED->vm_ref->srate();
    
    // get arg
    t_CKDUR v = GET_NEXT_DUR( ARGS );
    
    // get in microseconds
    t_CKINT usec = (t_CKINT)( v / srate * 1000000 );
    
    // make sure it's nonnegative
    if( usec < 0 ) usec = 0;
    
    // go
    RETURN->v_dur = TiltSensor_setPollRate( usec ) * srate / 1000000;
}

CK_DLL_SFUN( HidIn_cget_tiltPollRate )
{
    // get srate
    if( !SHRED || !SHRED->vm_ref )
    {
        // problem
        fprintf( stderr, "[chuck](via HID): can't get tiltPollRate on NULL shred/VM...\n" );
        RETURN->v_dur = 0;
        return;
    }
    t_CKFLOAT srate = SHRED->vm_ref->srate();
    
    RETURN->v_dur = TiltSensor_getPollRate() * srate / 1000000;
}

CK_DLL_SFUN( HidIn_start_cursor_track )
{
    RETURN->v_int = !Mouse_start_cursor_track();
}

CK_DLL_SFUN( HidIn_stop_cursor_track )
{
    RETURN->v_int = !Mouse_stop_cursor_track();
}

//-----------------------------------------------------------------------------
// HidOut API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( HidOut_ctor )
{
    OBJ_MEMBER_INT(SELF, HidOut_offset_data) = (t_CKUINT)new HidOut;
}

CK_DLL_DTOR( HidOut_dtor )
{
    delete (HidOut *)OBJ_MEMBER_INT(SELF, HidOut_offset_data);
    OBJ_MEMBER_INT(SELF, HidOut_offset_data) = 0;
}

CK_DLL_MFUN( HidOut_open )
{
    HidOut * mout = (HidOut *)OBJ_MEMBER_INT(SELF, HidOut_offset_data);
    t_CKINT num = GET_CK_INT(ARGS);
    RETURN->v_int = mout->open( num );
}

CK_DLL_MFUN( HidOut_good )
{
    HidOut * mout = (HidOut *)OBJ_MEMBER_INT(SELF, HidOut_offset_data);
    RETURN->v_int = (t_CKINT)mout->good();
}

CK_DLL_MFUN( HidOut_num )
{
    HidOut * mout = (HidOut *)OBJ_MEMBER_INT(SELF, HidOut_offset_data);
    RETURN->v_int = mout->num();
}

CK_DLL_MFUN( HidOut_name )
{
    HidOut * mout = (HidOut *)OBJ_MEMBER_INT(SELF, HidOut_offset_data);
    // TODO: memory leak, please fix, Thanks.
    Chuck_String * a = (Chuck_String *)instantiate_and_initialize_object( &t_string, NULL );
    // only if valid
    // if( mout->good() )
    //     a->str = mout->mout->getPortName( mout->num() );
    RETURN->v_string = a;
}

CK_DLL_MFUN( HidOut_printerr )
{
    HidOut * mout = (HidOut *)OBJ_MEMBER_INT(SELF, HidOut_offset_data);
    t_CKINT print_or_not = GET_CK_INT(ARGS);
    mout->set_suppress( !print_or_not );
}

CK_DLL_MFUN( HidOut_send )
{
    HidOut * mout = (HidOut *)OBJ_MEMBER_INT(SELF, HidOut_offset_data);
    Chuck_Object * fake_msg = GET_CK_OBJECT(ARGS);
    HidMsg the_msg;
/*    the_msg.data[0] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, HidMsg_offset_data1);
    the_msg.data[1] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, HidMsg_offset_data2);
    the_msg.data[2] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, HidMsg_offset_data3);
*/    RETURN->v_int = mout->send( &the_msg );
}


#ifndef __DISABLE_MIDI__

//-----------------------------------------------------------------------------
// MidiRW API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( MidiRW_ctor )
{
    OBJ_MEMBER_INT(SELF, MidiRW_offset_data) = (t_CKUINT)new MidiRW;
}

CK_DLL_DTOR( MidiRW_dtor )
{
    delete (MidiRW *)OBJ_MEMBER_INT(SELF, MidiRW_offset_data);
    OBJ_MEMBER_INT(SELF, MidiRW_offset_data) = 0;
}

CK_DLL_MFUN( MidiRW_open )
{
    MidiRW * mrw = (MidiRW *)OBJ_MEMBER_INT(SELF, MidiRW_offset_data);
    const char * filename = GET_CK_STRING(ARGS)->str.c_str();
    RETURN->v_int = mrw->open( filename );
}

CK_DLL_MFUN( MidiRW_close )
{
    MidiRW * mrw = (MidiRW *)OBJ_MEMBER_INT(SELF, MidiRW_offset_data);
    RETURN->v_int = mrw->close();
}

CK_DLL_MFUN( MidiRW_read )
{
    MidiRW * mrw = (MidiRW *)OBJ_MEMBER_INT(SELF, MidiRW_offset_data);
    Chuck_Object * fake_msg = GET_NEXT_OBJECT(ARGS);
    MidiMsg the_msg;
    t_CKTIME time = 0.0;
    RETURN->v_int = mrw->read( &the_msg, &time );
    OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data1) = the_msg.data[0];
    OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data2) = the_msg.data[1];
    OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data3) = the_msg.data[2];
    OBJ_MEMBER_TIME(fake_msg, MidiMsg_offset_when) = time;
}

CK_DLL_MFUN( MidiRW_write )
{
    MidiRW * mrw = (MidiRW *)OBJ_MEMBER_INT(SELF, MidiRW_offset_data);
    Chuck_Object * fake_msg = GET_NEXT_OBJECT(ARGS);
    t_CKTIME time = GET_NEXT_TIME(ARGS);
    MidiMsg the_msg;
    the_msg.data[0] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data1);
    the_msg.data[1] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data2);
    the_msg.data[2] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data3);
    RETURN->v_int = mrw->write( &the_msg, &time );
}


//-----------------------------------------------------------------------------
// MidiMsgOut API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( MidiMsgOut_ctor )
{
    OBJ_MEMBER_INT(SELF, MidiMsgOut_offset_data) = (t_CKUINT)new MidiMsgOut;
}

CK_DLL_DTOR( MidiMsgOut_dtor )
{
    delete (MidiMsgOut *)OBJ_MEMBER_INT(SELF, MidiMsgOut_offset_data);
    OBJ_MEMBER_INT(SELF, MidiMsgOut_offset_data) = 0;
}

CK_DLL_MFUN( MidiMsgOut_open )
{
    MidiMsgOut * mrw = (MidiMsgOut *)OBJ_MEMBER_INT(SELF, MidiMsgOut_offset_data);
    const char * filename = GET_CK_STRING(ARGS)->str.c_str();
    RETURN->v_int = mrw->open( filename );
}

CK_DLL_MFUN( MidiMsgOut_close )
{
    MidiMsgOut * mrw = (MidiMsgOut *)OBJ_MEMBER_INT(SELF, MidiMsgOut_offset_data);
    RETURN->v_int = mrw->close();
}

CK_DLL_MFUN( MidiMsgOut_write )
{
    MidiMsgOut * mrw = (MidiMsgOut *)OBJ_MEMBER_INT(SELF, MidiMsgOut_offset_data);
    Chuck_Object * fake_msg = GET_NEXT_OBJECT(ARGS);
    t_CKTIME time = GET_NEXT_TIME(ARGS);
    MidiMsg the_msg;
    the_msg.data[0] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data1);
    the_msg.data[1] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data2);
    the_msg.data[2] = (t_CKBYTE)OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data3);
    RETURN->v_int = mrw->write( &the_msg, &time );
}


//-----------------------------------------------------------------------------
// MidiMsgIn API
//-----------------------------------------------------------------------------
CK_DLL_CTOR( MidiMsgIn_ctor )
{
    OBJ_MEMBER_INT(SELF, MidiMsgIn_offset_data) = (t_CKUINT)new MidiMsgIn;
}

CK_DLL_DTOR( MidiMsgIn_dtor )
{
    delete (MidiMsgIn *)OBJ_MEMBER_INT(SELF, MidiMsgIn_offset_data);
    OBJ_MEMBER_INT(SELF, MidiMsgIn_offset_data) = 0;
}

CK_DLL_MFUN( MidiMsgIn_open )
{
    MidiMsgIn * mrw = (MidiMsgIn *)OBJ_MEMBER_INT(SELF, MidiMsgIn_offset_data);
    const char * filename = GET_CK_STRING(ARGS)->str.c_str();
    RETURN->v_int = mrw->open( filename );
}

CK_DLL_MFUN( MidiMsgIn_close )
{
    MidiMsgIn * mrw = (MidiMsgIn *)OBJ_MEMBER_INT(SELF, MidiMsgIn_offset_data);
    RETURN->v_int = mrw->close();
}

CK_DLL_MFUN( MidiMsgIn_read )
{
    MidiMsgIn * mrw = (MidiMsgIn *)OBJ_MEMBER_INT(SELF, MidiMsgIn_offset_data);
    Chuck_Object * fake_msg = GET_NEXT_OBJECT(ARGS);
    MidiMsg the_msg;
    t_CKTIME time = 0.0;
    RETURN->v_int = mrw->read( &the_msg, &time );
    OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data1) = the_msg.data[0];
    OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data2) = the_msg.data[1];
    OBJ_MEMBER_INT(fake_msg, MidiMsg_offset_data3) = the_msg.data[2];
    OBJ_MEMBER_TIME(fake_msg, MidiMsg_offset_when) = time;
}

#endif // __DISABLE_MIDI__
