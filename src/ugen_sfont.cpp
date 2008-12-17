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
// file: ugen_sfont.cpp
// desc: SoundFont Unit Generators
//
// author: Kyle Super (ksuper@princeton.edu)
// date: Spring 2007
//----------------------------------------------------------------------------

#include "ugen_sfont.h"
#include "chuck_type.h"
#include "util_math.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <fluidsynth.h>

static t_CKUINT g_srate             = 0;
static t_CKUINT sfont_offset_data   = 0;

static float PI     = (float)(4. * atan( 1. ));
static float TWOPI  = (float)(8. * atan( 1. ));

//----------------------------------------------------------------------------
// name: sfont_query()
// desc: Setup SoundFont Unit Generators
//----------------------------------------------------------------------------
DLL_QUERY sfont_query( Chuck_DL_Query * QUERY ) {
    g_srate = QUERY->srate;
    Chuck_Env * env = Chuck_Env::instance();
    Chuck_DL_Func * func = NULL;
    type_engine_register_deprecate( env, "sfont", "SFont" );

	//---------------------------------------------------------------------
    // init as base class: SFont
    //---------------------------------------------------------------------
    if( !type_engine_import_ugen_begin( env, "SFont", "UGen", env->global(), 
                                        sfont_ctor, sfont_dtor, sfont_tick, NULL ) )
        return FALSE;
    
    // member variable
    sfont_offset_data = type_engine_import_mvar( env, "int", "@sfont_data", FALSE );
    if( sfont_offset_data == CK_INVALID_OFFSET ) goto error;
    
    // add ctrl/cget: channel
    func = make_new_mfun( "int", "channel", sfont_ctrl_channel );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error; 
    func = make_new_mfun( "int", "channel", sfont_cget_channel );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: freq
    func = make_new_mfun( "float", "freq", sfont_ctrl_key );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "freq", sfont_cget_key );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: veloc
    //func = make_new_mfun( "int", "veloc", sfont_ctrl_veloc );
    //func->add_arg( "int", "value" );
    //if( !type_engine_import_mfun( env, func ) ) goto error;    
    //func = make_new_mfun( "int", "veloc", sfont_cget_veloc );
    //if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: read
    func = make_new_mfun( "string", "read", sfont_ctrl_read );
    func->add_arg( "string", "read" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: noteon
    func = make_new_mfun( "float", "noteOn", sfont_ctrl_noteon );
	func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: noteoff
    func = make_new_mfun( "float", "noteOff", sfont_ctrl_noteoff );
	func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: norm
    func = make_new_mfun( "float", "norm", sfont_ctrl_norm );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "norm", sfont_cget_norm );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: midi_chan
    func = make_new_mfun( "int", "midiChan", sfont_ctrl_midi_chan );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "int", "midiChan", sfont_cget_midi_chan );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: reverb
    func = make_new_mfun( "int", "reverb", sfont_ctrl_reverb );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "int", "reverb", sfont_cget_reverb );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: chorus
    func = make_new_mfun( "int", "chorus", sfont_ctrl_chorus );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "int", "chorus", sfont_cget_chorus );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: interp
    func = make_new_mfun( "int", "interp", sfont_ctrl_interp );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "int", "interp", sfont_cget_interp );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: reverb_room
    func = make_new_mfun( "float", "reverbRoom", sfont_ctrl_reverb_room );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "reverbRoom", sfont_cget_reverb_room );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: reverb_damp
    func = make_new_mfun( "float", "reverbDamp", sfont_ctrl_reverb_damp );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "reverbDamp", sfont_cget_reverb_damp );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: reverb_width
    func = make_new_mfun( "float", "reverbWidth", sfont_ctrl_reverb_width );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "reverbWidth", sfont_cget_reverb_width );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: reverb_level
    func = make_new_mfun( "float", "reverbLevel", sfont_ctrl_reverb_level );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "reverbLevel", sfont_cget_reverb_level );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: chorus_delay
    func = make_new_mfun( "int", "chorusDelay", sfont_ctrl_chorus_delay );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "int", "chorusDelay", sfont_cget_chorus_delay );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: chorus_level
    func = make_new_mfun( "float", "chorusLevel", sfont_ctrl_chorus_level );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "chorusLevel", sfont_cget_chorus_level );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: chorus_speed
    func = make_new_mfun( "float", "chorusSpeed", sfont_ctrl_chorus_speed );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "chorusSpeed", sfont_cget_chorus_speed );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: chorus_depth
    func = make_new_mfun( "float", "chorusDepth", sfont_ctrl_chorus_depth );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "chorusDepth", sfont_cget_chorus_depth );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: chorus_type
    func = make_new_mfun( "int", "chorusType", sfont_ctrl_chorus_type );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "int", "chorusType", sfont_cget_chorus_type );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: midi
    func = make_new_mfun( "string", "midiRead", sfont_ctrl_midi );
    func->add_arg( "string", "read" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: midi_play
    func = make_new_mfun( "void", "midiPlay", sfont_ctrl_midi_play );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: midi_stop
    func = make_new_mfun( "void", "midiStop", sfont_ctrl_midi_stop );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl/cget: midi_bpm
    func = make_new_mfun( "int", "midiBPM", sfont_ctrl_midi_bpm );
    func->add_arg( "int", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "int", "midiBPM", sfont_cget_midi_bpm );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: cc
    func = make_new_mfun( "void", "controlChange", sfont_ctrl_cc );
	func->add_arg( "int", "ctrl" );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: pbend
    func = make_new_mfun( "void", "pbend", sfont_ctrl_pbend );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: wheel
    func = make_new_mfun( "void", "wheel", sfont_ctrl_wheel );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: program
    func = make_new_mfun( "void", "program", sfont_ctrl_program );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: bank
    func = make_new_mfun( "void", "bank", sfont_ctrl_bank );
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // add ctrl: reset
    func = make_new_mfun( "void", "reset", sfont_ctrl_reset );
    if( !type_engine_import_mfun( env, func ) ) goto error;

	// add ctrl/cget: pan
    func = make_new_mfun( "float", "pan", sfont_ctrl_pan );
    func->add_arg( "float", "value" );
    if( !type_engine_import_mfun( env, func ) ) goto error;    
    func = make_new_mfun( "float", "pan", sfont_cget_pan);
    if( !type_engine_import_mfun( env, func ) ) goto error;
    
    // end the class import
    if( !type_engine_import_class_end( env ) )
        return FALSE;
    
    // end class imports
    return TRUE;
error:
    type_engine_import_class_end( env );
    return FALSE;
}

//----------------------------------------------------------------------------
// name: sfont
// desc: SoundFont Unit Generator
//----------------------------------------------------------------------------
struct sfont_data {
    
    // declarations
    fluid_settings_t * settings;
    fluid_synth_t * synth;
    fluid_player_t * player;
    int channel, id, midi_chan, reverb, chorus, interp, chorus_n, chorus_t, bpm;
    bool init, read, midi;
    double gain, reverb_r, reverb_d, reverb_w, reverb_l, chorus_l, chorus_s, chorus_d, pan, key;
    
    // constructor
    sfont_data() {
        settings = NULL;
        synth = NULL;
        player = NULL;
        channel = 0;
        id = 0;
        midi_chan = 16;
        reverb = 1;
        chorus = 1;
        interp = 2;
        chorus_n = 3;
        chorus_t = 0;
        bpm = 125;
        init = false;
        read = false;
        midi = false;
        gain = 0.2;
        reverb_r = 0.2;
        reverb_d = 0.0;
        reverb_w = 0.5;
        reverb_l = 0.9;
        chorus_l = 2.0;
        chorus_s = 0.3;
        chorus_d = 8.0;
		pan		 = 0.0;
		key		 = 0.0;
    }
};

//----------------------------------------------------------------------------
// name: sfont_ctor()
// desc: Create SoundFont Data
//----------------------------------------------------------------------------
CK_DLL_CTOR( sfont_ctor ) {
    sfont_data * d = new sfont_data;
    OBJ_MEMBER_UINT( SELF, sfont_offset_data ) = (t_CKUINT)d;
}

//----------------------------------------------------------------------------
// name: sfont_dtor()
// desc: Destroy SoundFont Data
//----------------------------------------------------------------------------
CK_DLL_DTOR( sfont_dtor ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    
    // delete data structures
    if( d->synth != NULL ) {
        delete_fluid_synth( d->synth );
        d->synth = NULL;
    }
    if( d->settings != NULL ) {
        delete_fluid_settings( d->settings );
        d->settings = NULL;
    }
    if( d->player != NULL ) {
        delete_fluid_player( d->player );
        d->player = NULL;
    }
    delete (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
}

//----------------------------------------------------------------------------
// name: fluid_synth_init
// desc: Initialize FluidSynth
//----------------------------------------------------------------------------
static void fluid_synth_init( sfont_data * d ) {
    
    // delete prior data structures
    d->init = false;
    if( d->synth != NULL ) {
        delete_fluid_synth( d->synth );
        d->synth = NULL;
    }
    if( d->settings != NULL ) {
        delete_fluid_settings( d->settings );
        d->settings = NULL;
    }
    
    // create new data structures
    d->settings = new_fluid_settings();
    if( d->settings == NULL ) {
        fprintf( stderr, "[chuck](via sfont): Failed to create the settings\n" );
        exit(-1);
    }
    d->synth = new_fluid_synth( d->settings );
    if( d->synth == NULL ) {
        fprintf( stderr, "[chuck](via sfont): Failed to create the synthesizer\n" );
        exit(-1);
    }
    d->init = true;
    
    // set various settings
    fluid_settings_setnum( d->settings, "synth.sample-rate", g_srate );
    fluid_settings_setnum( d->settings, "synth.gain", d->gain );
    fluid_settings_getnum( d->settings, "synth.gain", &(d->gain) );
    fluid_settings_setint( d->settings, "synth.midi-channels", d->midi_chan );
    fluid_settings_getint( d->settings, "synth.midi-channels", &(d->midi_chan) );
    if( d->reverb )
        fluid_settings_setstr( d->settings, "synth.reverb.active", "yes" );
    else
        fluid_settings_setstr( d->settings, "synth.reverb.active", "no" );
    if( d->chorus )
        fluid_settings_setstr( d->settings, "synth.chorus.active", "yes" );
    else
        fluid_settings_setstr( d->settings, "synth.chorus.active", "no" );
    if( d->interp == 0 )
        fluid_synth_set_interp_method( d->synth, -1, 0 );
    else if( d->interp == 1 )
        fluid_synth_set_interp_method( d->synth, -1, 1 );
    else if( d->interp == 2 )
        fluid_synth_set_interp_method( d->synth, -1, 4 );
    else
        fluid_synth_set_interp_method( d->synth, -1, 7 );
    fluid_synth_set_reverb( d->synth, d->reverb_r, d->reverb_d, d->reverb_w, d->reverb_l );
    fluid_synth_set_chorus( d->synth, d->chorus_n, d->chorus_l, d->chorus_s, d->chorus_d, d->chorus_t );
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_channel()
// desc: Set Note Channel
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_channel ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT channel = GET_CK_INT( ARGS );
    d->channel = channel;
    RETURN->v_int = d->channel;
}

//----------------------------------------------------------------------------
// name: sfont_cget_channel()
// desc: Get Note Channel
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_channel ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->channel;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_key()
// desc: Set Note Key
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_key ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
	float key = GET_CK_FLOAT( ARGS );
    d->key = key;
    RETURN->v_float = d->key;
}

//----------------------------------------------------------------------------
// name: sfont_cget_key()
// desc: Get Note Key
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_key ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->key;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_veloc()
// desc: Set Note Velocity
//----------------------------------------------------------------------------
//CK_DLL_CTRL( sfont_ctrl_veloc ) {
//    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
//    t_CKINT velocity = GET_CK_INT( ARGS );
//    d->velocity = velocity;
//    RETURN->v_int = d->velocity;
//}

//----------------------------------------------------------------------------
// name: sfont_cget_veloc()
// desc: Get Note Velocity
//----------------------------------------------------------------------------
//CK_DLL_CTRL( sfont_cget_veloc ) {
//    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
//    RETURN->v_int = d->velocity;
//}

//----------------------------------------------------------------------------
// name: sfont_ctrl_norm()
// desc: Set Gain Factor
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_norm ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double gain = GET_CK_FLOAT( ARGS );
    if( gain < 0.0 || gain > 10.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper gain value\n" );
    } else {
        if( d->init ) {
            fluid_settings_setnum( d->settings, "synth.gain", gain );
            fluid_settings_getnum( d->settings, "synth.gain", &(d->gain) );
        } else {
            d->gain = gain;
        }
    }
    RETURN->v_float = d->gain;
}

//----------------------------------------------------------------------------
// name: sfont_cget_norm()
// desc: Get Gain Factor
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_norm ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->gain;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_midi_chan()
// desc: Set Midi Channels
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_midi_chan ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT midi_chan = GET_CK_INT( ARGS );
    if( midi_chan < 16 || midi_chan > 256 ) {
        fprintf( stderr, "[chuck](via sfont): Improper number of midi channels\n" );
    } else {
        if( d->init ) {
            fluid_settings_setint( d->settings, "synth.midi-channels", midi_chan );
            fluid_settings_getint( d->settings, "synth.midi-channels", &(d->midi_chan) );
        } else {
            d->midi_chan = midi_chan;
        }
    }
    RETURN->v_int = d->midi_chan;
}

//----------------------------------------------------------------------------
// name: sfont_cget_midi_chan()
// desc: Get Midi Channels
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_midi_chan ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->midi_chan;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_reverb
// desc: Set Reverb
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_reverb ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT reverb = GET_CK_INT( ARGS );
    if( reverb < 0 || reverb > 1 ) {
        fprintf( stderr, "[chuck](via sfont): Improper reverb value\n" );
    } else {
        if( d->init ) {
            if( reverb )
                fluid_settings_setstr( d->settings, "synth.reverb.active", "yes" );
            else
                fluid_settings_setstr( d->settings, "synth.reverb.active", "no" );
        }
        d->reverb = reverb;
    }
    RETURN->v_int = d->reverb;
}

//----------------------------------------------------------------------------
// name: sfont_cget_reverb()
// desc: Get Reverb
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_reverb ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->reverb;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_chorus
// desc: Set Chorus
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_chorus ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT chorus = GET_CK_INT( ARGS );
    if( chorus < 0 || chorus > 1 ) {
        fprintf( stderr, "[chuck](via sfont): Improper chorus value\n" );
    } else {
        if( d->init ) {
            if( chorus )
                fluid_settings_setstr( d->settings, "synth.chorus.active", "yes" );
            else
                fluid_settings_setstr( d->settings, "synth.chorus.active", "no" );
        }
        d->chorus = chorus;
    }
    RETURN->v_int = d->chorus;
}

//----------------------------------------------------------------------------
// name: sfont_cget_chorus()
// desc: Get Chorus
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_chorus ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->chorus;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_interp
// desc: Set Interp Method
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_interp ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT interp = GET_CK_INT( ARGS );
    if( interp < 0 || interp > 3 ) {
        fprintf( stderr, "[chuck](via sfont): Improper interp method\n" );
    } else {
        if( d->init ) {
            if( interp == 0 )
                fluid_synth_set_interp_method( d->synth, -1, 0 );
            else if( interp == 1 )
                fluid_synth_set_interp_method( d->synth, -1, 1 );
            else if( interp == 2 )
                fluid_synth_set_interp_method( d->synth, -1, 4 );
            else
                fluid_synth_set_interp_method( d->synth, -1, 7 );
        }
        d->interp = interp;
    }
    RETURN->v_int = d->interp;
}

//----------------------------------------------------------------------------
// name: sfont_cget_interp()
// desc: Get Interp Method
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_interp ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->interp;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_reverb_room()
// desc: Set Reverb Room Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_reverb_room ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double val = GET_CK_FLOAT( ARGS );
    if( val < 0.0 || val > 1.2 ) {
        fprintf( stderr, "[chuck](via sfont): Improper room value\n" );
    } else {
        d->reverb_r = val;
        if( d->init )
            fluid_synth_set_reverb( d->synth, d->reverb_r, d->reverb_d, d->reverb_w, d->reverb_l );
    }
    RETURN->v_float = d->reverb_r;
}

//----------------------------------------------------------------------------
// name: sfont_cget_reverb_room
// desc: Get Reverb Room Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_reverb_room ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->reverb_r;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_reverb_damp()
// desc: Set Reverb Damp Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_reverb_damp ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double val = GET_CK_FLOAT( ARGS );
    if( val < 0.0 || val > 1.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper damp value\n" );
    } else {
        d->reverb_d = val;
        if( d->init )
            fluid_synth_set_reverb( d->synth, d->reverb_r, d->reverb_d, d->reverb_w, d->reverb_l );
    }
    RETURN->v_float = d->reverb_d;
}

//----------------------------------------------------------------------------
// name: sfont_cget_reverb_damp
// desc: Get Reverb damp Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_reverb_damp ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->reverb_d;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_reverb_width()
// desc: Set Reverb Width Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_reverb_width ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double val = GET_CK_FLOAT( ARGS );
    if( val < 0.0 || val > 100.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper width value\n" );
    } else {
        d->reverb_w = val;
        if( d->init )
            fluid_synth_set_reverb( d->synth, d->reverb_r, d->reverb_d, d->reverb_w, d->reverb_l );
    }
    RETURN->v_float = d->reverb_w;
}

//----------------------------------------------------------------------------
// name: sfont_cget_reverb_width
// desc: Get Reverb Width Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_reverb_width ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->reverb_w;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_reverb_level()
// desc: Set Reverb Level Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_reverb_level ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double val = GET_CK_FLOAT( ARGS );
    if( val < -30.0 || val > 30.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper reverb level value\n" );
    } else {
        d->reverb_l = val;
        if( d->init )
            fluid_synth_set_reverb( d->synth, d->reverb_r, d->reverb_d, d->reverb_w, d->reverb_l );
    }
    RETURN->v_float = d->reverb_l;
}

//----------------------------------------------------------------------------
// name: sfont_cget_reverb_level
// desc: Get Reverb Level Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_reverb_level ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->reverb_l;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_chorus_delay
// desc: Set Chorus Delay
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_chorus_delay ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT val = GET_CK_INT( ARGS );
    if( val < 0 || val > 99 ) {
        fprintf( stderr, "[chuck](via sfont): Improper chorus delay value\n" );
    } else {
        d->chorus_n = val;
        if( d->init )
            fluid_synth_set_chorus( d->synth, d->chorus_n, d->chorus_l, d->chorus_s, d->chorus_d, d->chorus_t );
    }
    RETURN->v_int = d->chorus_n;
}

//----------------------------------------------------------------------------
// name: sfont_cget_chorus_delay()
// desc: Get Chorus Delay
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_chorus_delay ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->chorus_n;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_chorus_level()
// desc: Set Chorus Level Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_chorus_level ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double val = GET_CK_FLOAT( ARGS );
    if( val < 0.0 || val > 10.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper chorus level value\n" );
    } else {
        d->chorus_l = val;
        if( d->init )
            fluid_synth_set_chorus( d->synth, d->chorus_n, d->chorus_l, d->chorus_s, d->chorus_d, d->chorus_t );
    }
    RETURN->v_float = d->chorus_l;
}

//----------------------------------------------------------------------------
// name: sfont_cget_chorus_level
// desc: Get Chorus Level Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_chorus_level ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->chorus_l;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_chorus_speed()
// desc: Set Chorus Speed Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_chorus_speed ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double val = GET_CK_FLOAT( ARGS );
    if( val < 0.29 || val > 5.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper chorus speed value\n" );
    } else {
        d->chorus_s = val;
        if( d->init )
            fluid_synth_set_chorus( d->synth, d->chorus_n, d->chorus_l, d->chorus_s, d->chorus_d, d->chorus_t );
    }
    RETURN->v_float = d->chorus_s;
}

//----------------------------------------------------------------------------
// name: sfont_cget_chorus_speed()
// desc: Get Chorus Speed Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_chorus_speed ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->chorus_s;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_chorus_depth()
// desc: Set Chorus Depth Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_chorus_depth ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double val = GET_CK_FLOAT( ARGS );
    if( val < 0.0 || val > 46.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper chorus depth value\n" );
    } else {
        d->chorus_d = val;
        if( d->init )
            fluid_synth_set_chorus( d->synth, d->chorus_n, d->chorus_l, d->chorus_s, d->chorus_d, d->chorus_t );
    }
    RETURN->v_float = d->chorus_d;
}

//----------------------------------------------------------------------------
// name: sfont_cget_chorus_depth
// desc: Get Chorus Depth Size
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_chorus_depth ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->chorus_d;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_chorus_type
// desc: Set Chorus Type
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_chorus_type ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT val = GET_CK_INT( ARGS );
    if( val < 0 || val > 1 ) {
        fprintf( stderr, "[chuck](via sfont): Improper chorus type value\n" );
    } else {
        d->chorus_t = val;
        if( d->init )
            fluid_synth_set_chorus( d->synth, d->chorus_n, d->chorus_l, d->chorus_s, d->chorus_d, d->chorus_t );
    }
    RETURN->v_int = d->chorus_t;
}

//----------------------------------------------------------------------------
// name: sfont_cget_chorus_type()
// desc: Get Chorus Type
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_chorus_type ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->chorus_t;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_read()
// desc: Read SoundFont File
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_read ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    const char * filename = GET_CK_STRING( ARGS )->str.c_str();
    
    // initialize data structures
    if( !d->init )
        fluid_synth_init( d );
    
    // delete prior data
    if( fluid_synth_sfcount( d->synth ) ) {
        if( fluid_synth_sfunload( d->synth, d->id, true ) == -1 ) {
            fprintf( stderr, "[chuck](via sfont): Unable to unload soundfont\n" );
            return;
        }
    }
    d->read = false;
    
    // load new soundfont
    if( fluid_is_soundfont( (char *)filename ) ) {
        d->id = fluid_synth_sfload( d->synth, filename, true );
        if( d->id == -1 ) {
            fprintf( stderr, "[chuck](via sfont): Unable to load soundfont\n" );
            return;
        }
    } else {
        fprintf( stderr, "[chuck](via sfont): Not a soundfont file\n" );
        return;
    }
    d->read = true;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_noteon()
// desc: Turn Note On
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_noteon ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
	float val = GET_CK_FLOAT( ARGS );
    if( d->init && d->read )
        fluid_synth_noteon( d->synth, d->channel, ftom( d->key ), val );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" ); 
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_noteoff()
// desc: Turn Note Off
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_noteoff ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->init && d->read )
        fluid_synth_noteoff( d->synth, d->channel, ftom( d->key ) );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" );
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_midi()
// desc: Read midi File
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_midi ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    const char * filename = GET_CK_STRING( ARGS )->str.c_str();
    
    // delete prior data
    if( d->player != NULL ) {
        delete_fluid_player( d->player );
        d->player = NULL;
    }
    d->midi = false;
    
    // check prereqs
    if( !d->init ) {
        fprintf( stderr, "[chuck](via sfont): Load SoundFont first\n" );
        return;
    }
    
    // load new midi player
    if( fluid_is_midifile( (char *)filename ) ) {
        d->player = new_fluid_player( d->synth );
        if( d->player == NULL ) {
            fprintf( stderr, "[chuck](via sfont): Error creating midi player\n" );
            return;
        }
        fluid_player_add( d->player, (char *)filename );
    } else {
        fprintf( stderr, "[chuck](via sfont): Not a midi file\n" );
        return;
    }
    d->midi = true;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_midi_play()
// desc: Turn Midi On
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_midi_play ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->midi )
        fluid_player_play( d->player );
    else
        fprintf( stderr, "[chuck](via sfont): Load midi file first\n" );
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_midi_stop()
// desc: Turn Midi Off
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_midi_stop ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->midi ) {
        delete_fluid_player( d->player );
        d->player = NULL;
        d->midi = false;
        for( int i = 0; i < d->midi_chan; i++ )
            fluid_synth_cc( d->synth, i, 123, 0 ); 
    } else
        fprintf( stderr, "[chuck](via sfont): Load midi file first\n" );
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_midi_bpm
// desc: Set Midi bpm
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_midi_bpm ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    t_CKINT val = GET_CK_INT( ARGS );
    if( val < 0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper midi bpm value\n" );
    } else {
        d->bpm = val;
        if( d->player )
            fluid_player_set_bpm( d->player, d->bpm );
    }
    RETURN->v_int = d->bpm;
}

//----------------------------------------------------------------------------
// name: sfont_cget_midi_bpm()
// desc: Get Midi bpm
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_midi_bpm ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_int = d->bpm;
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_cc
// desc: Control Change
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_cc ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
	t_CKINT   i = GET_NEXT_INT(ARGS);
    t_CKFLOAT f = GET_NEXT_FLOAT(ARGS);
    if( d->init && d->read )
        fluid_synth_cc( d->synth, d->channel, i, f );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" ); 
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_pbend
// desc: Control Pitch Bend
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_pbend ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->init && d->read )
        fluid_synth_pitch_bend( d->synth, d->channel, ftom( d->key ) );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" ); 
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_wheel
// desc: Control Wheel Sensitivity
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_wheel ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->init && d->read )
        fluid_synth_pitch_wheel_sens( d->synth, d->channel, ftom( d->key ) );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" ); 
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_program
// desc: Program Change
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_program ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->init && d->read )
        fluid_synth_program_change( d->synth, d->channel, ftom( d->key ) );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" ); 
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_bank
// desc: Bank Select
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_bank ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->init && d->read )
        fluid_synth_bank_select( d->synth, d->channel, ftom( d->key ) );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" ); 
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_reset
// desc: Reset Patches
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_reset ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    if( d->init && d->read )
        fluid_synth_system_reset( d->synth );
    else
        fprintf( stderr, "[chuck](via sfont): Load SoundFont file first\n" ); 
}

//----------------------------------------------------------------------------
// name: sfont_ctrl_pan()
// desc: Set Pan Factor
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_ctrl_pan ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    double pan = GET_CK_FLOAT( ARGS );
    if( pan < -1.0 || pan > 1.0 ) {
        fprintf( stderr, "[chuck](via sfont): Improper pan value\n" );
    } else {
        d->pan = pan;
    }
    RETURN->v_float = d->pan;
}

//----------------------------------------------------------------------------
// name: sfont_cget_pan()
// desc: Get Pan Factor
//----------------------------------------------------------------------------
CK_DLL_CTRL( sfont_cget_pan ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    RETURN->v_float = d->pan;
}

//----------------------------------------------------------------------------
// name: sfont_tick()
// desc: Return Samples
//----------------------------------------------------------------------------
CK_DLL_TICK( sfont_tick ) {
    sfont_data * d = (sfont_data *)OBJ_MEMBER_UINT( SELF, sfont_offset_data );
    
    // generate audio samples
    SAMPLE lsample = 0.0;
    SAMPLE rsample = 0.0;
    if( d->init && d->read ) {
        if( fluid_synth_write_float( d->synth, 1, &lsample, 0, 0, &rsample, 0, 0 ) ) {
            fprintf( stderr, "[chuck](via sfont): Error creating audio samples\n" );
            lsample = 0.0;
            rsample = 0.0;
        }
    }
    
    // return audio samples
	double p = (d->pan + 1.0) / 2.0;
    *out = ((1-p)*lsample + p*rsample);
    return TRUE;
}
