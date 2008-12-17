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
----------------------------------------------------------------------------*/

//----------------------------------------------------------------------------
// file: ugen_sfont.h
// desc: ...
//
// author: Kyle Super (ksuper@princeton.edu)
// date: Spring 2007
//----------------------------------------------------------------------------

#ifndef __UGEN_SFONT_H__
#define __UGEN_SFONT_H__

#include "chuck_dl.h"

// query
DLL_QUERY sfont_query( Chuck_DL_Query * QUERY );

// sfont
CK_DLL_CTOR( sfont_ctor );
CK_DLL_DTOR( sfont_dtor );
CK_DLL_CTRL( sfont_ctrl_channel );
CK_DLL_CTRL( sfont_cget_channel );
CK_DLL_CTRL( sfont_ctrl_key );
CK_DLL_CTRL( sfont_cget_key );
//CK_DLL_CTRL( sfont_ctrl_veloc );
//CK_DLL_CTRL( sfont_cget_veloc );
CK_DLL_CTRL( sfont_ctrl_read );
CK_DLL_CTRL( sfont_ctrl_noteon );
CK_DLL_CTRL( sfont_ctrl_noteoff );
CK_DLL_CTRL( sfont_ctrl_norm );
CK_DLL_CTRL( sfont_cget_norm );
CK_DLL_CTRL( sfont_ctrl_midi_chan );
CK_DLL_CTRL( sfont_cget_midi_chan );
CK_DLL_CTRL( sfont_ctrl_reverb );
CK_DLL_CTRL( sfont_cget_reverb );
CK_DLL_CTRL( sfont_ctrl_chorus );
CK_DLL_CTRL( sfont_cget_chorus );
CK_DLL_CTRL( sfont_ctrl_interp );
CK_DLL_CTRL( sfont_cget_interp );
CK_DLL_CTRL( sfont_ctrl_reverb_room );
CK_DLL_CTRL( sfont_cget_reverb_room );
CK_DLL_CTRL( sfont_ctrl_reverb_damp );
CK_DLL_CTRL( sfont_cget_reverb_damp );
CK_DLL_CTRL( sfont_ctrl_reverb_width );
CK_DLL_CTRL( sfont_cget_reverb_width );
CK_DLL_CTRL( sfont_ctrl_reverb_level );
CK_DLL_CTRL( sfont_cget_reverb_level );
CK_DLL_CTRL( sfont_ctrl_chorus_delay );
CK_DLL_CTRL( sfont_cget_chorus_delay );
CK_DLL_CTRL( sfont_ctrl_chorus_level );
CK_DLL_CTRL( sfont_cget_chorus_level );
CK_DLL_CTRL( sfont_ctrl_chorus_speed );
CK_DLL_CTRL( sfont_cget_chorus_speed );
CK_DLL_CTRL( sfont_ctrl_chorus_depth );
CK_DLL_CTRL( sfont_cget_chorus_depth );
CK_DLL_CTRL( sfont_ctrl_chorus_type );
CK_DLL_CTRL( sfont_cget_chorus_type );
CK_DLL_CTRL( sfont_ctrl_midi );
CK_DLL_CTRL( sfont_ctrl_midi_play );
CK_DLL_CTRL( sfont_ctrl_midi_stop );
CK_DLL_CTRL( sfont_ctrl_midi_bpm );
CK_DLL_CTRL( sfont_cget_midi_bpm );
CK_DLL_CTRL( sfont_ctrl_cc );
CK_DLL_CTRL( sfont_ctrl_pbend );
CK_DLL_CTRL( sfont_ctrl_wheel );
CK_DLL_CTRL( sfont_ctrl_program );
CK_DLL_CTRL( sfont_ctrl_bank );
CK_DLL_CTRL( sfont_ctrl_reset );
CK_DLL_CTRL( sfont_ctrl_pan );
CK_DLL_CTRL( sfont_cget_pan );
CK_DLL_TICK( sfont_tick );

#endif
