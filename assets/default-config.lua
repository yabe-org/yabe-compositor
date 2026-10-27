yabe.config({
  pixels_per_meter = 2500.0,
})

yabe.keybind({ "SUPER", "Return" }, yabe.dispatchers.exec("weston-terminal"))
yabe.keybind({ "SUPER", "D" }, yabe.dispatchers.exec("wofi --show drun"))
yabe.keybind({ "SUPER", "C" }, yabe.dispatchers.recenter())

for i = 0, 9 do
  yabe.keybind({ "SUPER", tostring(i) }, yabe.dispatchers.set_workspace(i))
end
