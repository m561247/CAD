input LssdTLin LssdRPCin LssdBRin LssdBLin LssdTRin
input Faultolin InstrnFetch Phi1 Phi2 Phi3 Phi4
input B<0:31> LssdEn
set 0 LssdEn FaultolEn
set 0 Phi2 Phi3 Phi4
set 0 A#<0:7>
set 0 B#<0:7>
set 0 A2Mux#<0:3>
delay Phi1 0.0 -1.0
worst -c phi1.cmd
prg -t 10
many -n 10
spice spice1

clear
input LssdTLin LssdRPCin LssdBRin LssdBLin LssdTRin
input Faultolin InstrnFetch Phi1 Phi2 Phi3 Phi4
input B<0:31> LssdEn
precharged Col<0:63>
precharged *Bit#
set 0 LssdEn FaultolEn
set 0 Phi1 Phi3 Phi4
set 0 Reset
set 0 A#<0:7>
set 0 B#<0:7>
set 0 PadEnable#0 PadEnable#1
delay Phi2 0.0 -1.0
delay RqstPC<0> 0 0
delay RqstPC<1> 0 0
worst -c phi2.cmd
prg -t 10
many -n 10
spice spice2

clear
input LssdTLin LssdRPCin LssdBRin LssdBLin LssdTRin
input Faultolin InstrnFetch Phi1 Phi2 Phi3 Phi4
input B<0:31> LssdEn
set 0 LssdEn FaultolEn
set 0 Phi1 Phi2 Phi4
set 0 Xor#<0:7>
set 0 RefrMem*#0
set 0 RefReg#<0:1>
set 0 RefTagLo#0 RefTagLo#3 RefTagHi#1 RefTagHi#2
set 0 PadEnable#<0:1>
delay Phi3 0.0 -1.0
worst -c phi3.cmd
prg -t 10
many -n 10
spice spice3

clear
input LssdTLin LssdRPCin LssdBRin LssdBLin LssdTRin
input Faultolin InstrnFetch Phi1 Phi2 Phi3 Phi4
input B<0:31> LssdEn
set 0 LssdEn FaultolEn
set 0 Phi1 Phi2 Phi4
delay Phi3 -1.0 0.0
worst -c phi3b.cmd
prg -t 10
many -n 10

clear
input LssdTLin LssdRPCin LssdBRin LssdBLin LssdTRin
input Faultolin InstrnFetch Phi1 Phi2 Phi3 Phi4
input B<0:31> LssdEn
set 0 Phi1 Phi2 Phi3
set 0 LssdEn FaultolEn
delay Phi4 0.0 -1.0
worst -c phi4.cmd
prg -t 10
many -n 10
spice spice4

prcap -t 1 -c cap.cmd
prres -t 5000 -c res.cmd
