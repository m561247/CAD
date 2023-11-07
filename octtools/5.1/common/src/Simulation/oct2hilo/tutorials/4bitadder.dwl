waveform 4bitadder;
  base bin;
  input TinA[3:0]:=0;
  input TinB[3:0]:=0;
  input Tcin:=0;
  output Tcout;
  output Tsum[3:0];

begin

  TinA:= 0010;
  TinB:= 0010;
  Tcin:=1;
  TinA:= 0011;
  TinB:= 0011;
  TinA:= 0111;
  TinB:= 0111;
  TinA:= 1111;
  TinB:= 1111;
  Tcin:=0;

  TinA:= 1111;
  TinB:= 0000;
  TinB:= 0001;
  Tcin:=1;

  Tcin:=0;
  TinA:= 1111;
  TinB:= 0000;
  Tcin:=1;
  TinB:= 1111;

  Tcin:=0;
  TinA:= 1111
  TinB:= 0111;

  TinA:= 1111
  TinB:= 0010;
end
endwaveform
