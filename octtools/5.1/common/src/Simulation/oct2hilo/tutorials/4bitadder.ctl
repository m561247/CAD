mv inA inA<3:0>
mv inB inB<3:0>
mv sum sum<3:0>

ma shsigs
  sh inA inB cin sum cout
$end

se inA 0001
se inB 0001
se cin 0
ev
shsigs

se inB 1110
ev
shsigs

se cin 1
ev
shsigs

se inA 0011
se inB 1011
ev
shsigs

se cin 0
ev
shsigs
