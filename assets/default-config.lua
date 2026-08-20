yabe.config({
  pixels_per_meter = 2500.0,
})

yabe.keybind({ "SUPER", "Return" }, yabe.dispatchers.exec("weston-terminal"))
yabe.keybind({ "SUPER", "D" }, yabe.dispatchers.exec("wofi --show drun"))
