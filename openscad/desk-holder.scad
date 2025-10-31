$fn = 90;

module slot() {
  w = 30.2;
  d = 5.88;
  h = 10.0;
  translate([-w / 2, -d / 2, -h/2])
    cube([w, d, h]);
}

difference() {
  translate([-20, -30, 0])
  minkowski() {
    cube([40, 40, 8]);
    cylinder(.01, r = 2);
  }
  translate([0,0,8])
    rotate([20, 0, 0])
      slot();
}
