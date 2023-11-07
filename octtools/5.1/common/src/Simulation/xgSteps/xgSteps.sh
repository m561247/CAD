#!/bin/csh -f


if ( $#argv < 2 ) then
	echo "usage: xgSteps <filename> nodename nodename ..."
	exit 1
endif

set infile = $1
shift

set label = ( `awk '/Index/ { print ; exit }' $infile` )



echo "TitleText: junk" > tmp
echo "XUnitText: $label[2]" >> tmp



@ count = 3

while ($#argv ) 

    set name = $1
    @ count = 3
    while ( $count <= $#label ) 
	if ( $label[$count] == $name )  break
	@ count++
     end
     if ( $count <= $#label ) then
          echo "" >> tmp
	  echo \" $label[$count] >> tmp
	  set awkcom = '/^[0-9]/ { print $2, $'$count' }'
          awk "$awkcom" $infile >> tmp 
     else 
	  echo "Legal node names are $label" 
	  exit 1
     endif
    shift
end

xgraph  < tmp
