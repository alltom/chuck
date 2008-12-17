// patch
Brass brass => JCRev r => dac;
.75 => r.gain;
.1 => r.mix;

// our notes
[ 61, 63, 65, 66, 68 ] @=> int notes[];

// infinite time-loop
while( true )
{
    // set
    Std.rand2f( 0, 1 ) => brass.lip;
    Std.rand2f( 0, 1 ) => brass.slide;
    Std.rand2f( 0, 12 ) => brass.vibratoFreq;
    Std.rand2f( 0, 1 ) => brass.vibratoGain;
    Std.rand2f( 0, 1 ) => brass.volume;

    // print
    <<< "---", "" >>>;
    <<< "lip tension:", brass.lip() >>>;
    <<< "slide length:", brass.slide() >>>;
    <<< "vibrato freq:", brass.vibratoFreq() >>>;
    <<< "vibrato gain:", brass.vibratoGain() >>>;
    <<< "volume:", brass.volume() >>>;

    for( int i; i < notes.cap(); i++ )
    {
        play( 12 + notes[i], Std.rand2f( .6, .9 ) );
        300::ms => now;
    }
}

// basic play function (add more arguments as needed)
fun void play( float note, float velocity )
{
    // start the note
    Std.mtof( note ) => brass.freq;
    velocity => brass.noteOn;
}
