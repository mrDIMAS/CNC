out_name = getSaveFileName caption:"Export to lcode File" types:"lcode Files(*.lcode)|*.lcode" initialDir:(getDir #export) historyCategory:"ExportTextFiles"

if out_name != undefined then 
(
	file = createfile out_name 		

	lastX = 0
	lastY = 0
	
	for obj in selection do
	(
		Converttosplineshape obj	
					
		for pns = 1 to ( numSplines obj ) do
		(	
			for pnk = 1 to (( numknots obj pns ) - 1) do
			(
				knot_pos1 = getknotpoint  obj pns pnk
				knot_pos2 = getknotpoint  obj pns (pnk + 1)
				
				x0 = knot_pos1.x as integer
				y0 = -knot_pos1.y as integer
				
				x1 = knot_pos2.x as integer
				y1 = -knot_pos2.y as integer		
				
				if( x0 != lastX or y0 != lastY) then
				(
					format "U 100;\nL % % % %;\nD 100;\n" lastX lastY x0 y0 to:file
				)
				
				format "L % % % %;\n"  x0 y0 x1 y1 to:file
					
				lastX = x1
				lastY = y1
			)			
		)				
	)
	
	close file
	fclose file
)