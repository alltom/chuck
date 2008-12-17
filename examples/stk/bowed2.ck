// patch
Bowed bow => dac;

// scale
[0, 2, 4, 7, 8, 11] @=> int scale[];

// infinite time loop
while( true )
{
    Std.rand2f( 0, 128 ) => float pressure;
    Std.rand2f( 0, 128 ) => float position;
    Std.rand2f( 0, 128 ) => float vibratofreq;
    Std.rand2f( 0, 128 ) => float vibratogain;
    Std.rand2f( 0, 128 ) => float volume;

    <<< "---", "" >>>;
    <<< "bow pressure:", pressure >>>;
    <<< "bow position:", position >>>;
    <<< "vibrato freq:", vibratofreq >>>;
    <<< "vibrato gain:", vibratogain >>>;
    <<< "volume:", volume >>>;

    // bow pressure
    bow.controlChange( 2, pressure );
    // bow position
    bow.controlChange( 4, position );
    // vibrato freq
    bow.controlChange( 11, vibratofreq );
    // vibrato gain
    bow.controlChange( 1, vibratogain );
    // volume
    bow.controlChange( 128, volume );

    // set freq
    scale[Std.rand2(0,scale.cap()-1)] + 57 => Std.mtof => bow.freq;
    // go
    .8 => bow.noteOn;

    // advance time
    Std.rand2f(.8, 2)::second => now;
}
