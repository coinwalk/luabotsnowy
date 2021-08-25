y       = balance/64
nextbet = y
older   = balance
old     = balance
bolder  = balance
lol     = 1000000
x       = lol
target  = 1
chance  = 49.95
Low     = 0
High    = 499499
bethigh = false
bill    = lol
bi      = lol
vim     = false
bal     = balance

function dobet()
if win then
    bal  = bal+previousbet
    bill = bill-1
else
    bal  = bal-previousbet
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
 if (balance<=((older/64)*24)) then
     nextbet = balance/64
     x       = lol
     bill    = lol
     older   = balance
     vim     = true
 end
 if ((balance>=old) and (previousbet>=(y*2))) or ((balance>=bolder) and (vim==true)) then
    nextbet = balance/64
    bill    = lol
    x       = lol
    old     = balance
    older   = balance
    vim     = false
end
if bal>target or bal<nextbet then
        stop()
    end
end
