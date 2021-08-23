-- any key binding can be changed here, just write in Lua
-- what you want a key to do

-- you can use any valid lua code here
-- we have the following functions
-- Bet() does a bet with nextbet as betsize


-- number keys used to set certain chances
function key_0() if chance==9.99 then chance=95 else chance=9.99  end end

function key_1() chance=5 end

function key_2() if chance==49.95 then chance=47.5 else chance=49.95 end end

function key_3() chance=32 end

function key_4() if chance==23.75 then chance=24 else chance=23.75 end end

function key_5() chance=19.95 end

function key_6() chance=16 end

function key_7() chance=70 end

function key_8() chance=11.875 end

function key_9() chance=11 end

function key_A() basebet=nextbet end

function key_a() nextbet=basebet end

function key_apostrophe() print("you pressed ' - Edit your manual.lua to make it do something useful") end

function key_b() print("you pressed b - Edit your manual.lua to make it do something useful") end

function key_bracketleft() print("you pressed [ - Edit your manual.lua to make it do something useful") end

function key_bracketright() print("you pressed ] - Edit your manual.lua to make it do something useful") end

function key_comma() print("you pressed , - Edit your manual.lua to make it do something useful") end

function key_c() print("you pressed c - Edit your manual.lua to make it do something useful") end

function key_dot() print("you pressed . - Edit your manual.lua to make it do something useful") end

function key_d() print("key d is not doing anything but writing this msg, change manual.lua")end

-- function key_e() is not availble, the C engine uses the key already

function key_f() nextbet=nextbet-basebet end

function key_g() nextbet=nextbet+basebet end

function key_h() print("you pressed h - Edit your manual.lua to make it do something useful") end

function key_i() print("you pressed i - Edit your manual.lua to make it do something useful") end

-- function key_j() is not availble, the C engine uses the key already

function key_k() print ("you pressed k - Edit you manual.lua...") end

-- function key_l() is not availble, the C engine uses the key already

function key_m() nextbet=0.01 end

function key_n() print("you pressed n - Edit your manual.lua to make it do something useful") end

function key_numbersign() print("you pressed # - Edit your manual.lua to make it do something useful") end

function key_o() print("you pressed o - Edit your manual.lua to make it do something useful") end

-- function key_p() is not availble, the C engine uses the key already

-- function key_q() is not availble, the C engine uses the key already

-- function key_r() is not availble, the C engine uses the key already

function key_semicolon() print("you pressed ; - Edit your manual.lua to make it do something useful") end

function key_slash() print("you pressed / - Edit your manual.lua to make it do something useful") end

function key_s() nextbet=nextbet/4 end

function key_t() 
	if maxbalance==balance then 
		print("maxbalance reached")
		nextbet=0.01
	else
		nextbet=(maxbalance-balance)/((0.999/(chance/100)-1))/0.999
		if nextbet<0.01 then nextbet=0.01 end
	end
end

function key_u() print("you pressed u - Edit your manual.lua to make it do something useful") end

function key_v() print("you pressed v - Edit your manual.lua to make it do something useful") end

function key_w() nextbet=balance/1600 end

function key_x() nextbet=nextbet/2 end

-- function key_y() is not availble, the C engine uses the key already

function key_z() nextbet=nextbet*2 end


