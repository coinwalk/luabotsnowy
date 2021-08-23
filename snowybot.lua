cat     = 6400
y       = balance/6400
x       = y 
v       = y
chance  = 49.95
bethigh = false
nextbet = y
target  = 330
old     = balance
elf     = y
myseed  = 0
Low     = 0
lol     = balance
High    = 499499
bob     = y
myseed  = 0
kill    = 0
doh     = true
vim     = false

function dobet()    
    if win then
   elf = elf+previousbet
   bob = bob-previousbet
   else
   elf = elf-previousbet 
   bob = bob+previousbet  
 end  
  if bob<y then
    bob=y
    end
  if elf<y then
   elf=y
    end
if (elf>=(x*3)) then
     nextbet = previousbet*2
     x       = nextbet
     elf     = x 
     vim     = false
 end 
 if (bob>=(x*6)) then
     nextbet = previousbet*2
     x       = nextbet
     bob     = x 
     vim     = true
 end
 if  balance>old and (nextbet>=(y*4)) then
     y=balance/6400
     nextbet = y
     vim     = false
     doh     = true
     x       = y
     elf     = y
     bob     = y
     old     = balance
 end
 if nextbet<0.00000001 then
	 nextbet=0.00000001
 end
if balance>target then
        stop()
    end
end

