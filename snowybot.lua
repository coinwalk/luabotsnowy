y       = balance/512
nextbet = y
older   = balance
old     = balance
bolder  = balance
lol     = 1000000
x       = lol
target  = 10000
chance  = 49.95
Low     = 0
High    = 499499
bethigh = false
bill    = lol
bi      = lol
vim     = false

function dobet()
if win then
    bill = bill-1
else
    bill = bill+1
end  
 if (bill<=x-7) then
     nextbet = previousbet*2 
     x       = bill
     bill    = x
end 
 if (bill>=x+4) then
     nextbet = previousbet*2 
     x       = bill
     bill    = x  
 end 
 if (balance<=((older/512)*192)) then
     nextbet = balance/512
     x       = lol
     bill    = lol
     older   = balance
     vim     = true
 end
 if ((balance>=old) and (previousbet>=(y*2))) or ((balance>=bolder) and (vim==true)) then
    nextbet = balance/512
    bill    = lol
    x       = lol
    old     = balance
    older   = balance
    vim     = false
end
if balance>target or balance<nextbet then
        stop()
    end
end
