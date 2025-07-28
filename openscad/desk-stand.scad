use <torus.scad>;

$fn = 90;
inset = 2.0;
module pegs() {
  module peg() {
    h = 3.4;
    cylinder(h, d=2.0);
    translate([0, 0.5, h]) {
      rotate([90, 0, 0])
        linear_extrude(1) {
          polygon([[-1.3, 0], [0, 1], [1.3, 0]]);
      }
    }
  }
  translate([ inset,       inset,      0]) peg();
  translate([ inset,       30 - inset, 0]) peg();
  translate([ 30 - inset,  inset,      0]) peg();
  translate([ 30 - inset,  30 - inset, 0]) peg();
}

module screen() {
  w = 26.16 + 0.4;
  h = 29.2 + 0.4;
  translate([15 - w / 2, 15 - h / 2, 0.5]) {
    cube([w, h, 99]);
    translate([0,-0.3,0])
      cube([20, 2, 99]);
  }

  r_dogbone = 0.6;
  translate([15 - w / 2, 15 - h / 2, 0.5])
    cylinder(99, r = r_dogbone);
  translate([15 + w / 2, 15 - h / 2, 0.5])
    cylinder(99, r = r_dogbone);
  translate([15 - w / 2, 15 + h / 2, 0.5])
    cylinder(99, r = r_dogbone);
  translate([15 + w / 2, 15 + h / 2, 0.5])
    cylinder(99, r = r_dogbone);
}
module window() {
  r = 0.5;
  w = 23.40 + 1.0;
  translate([15, 15 + 2.1, 0])
    minkowski() {
      cube([w - 2 * r, w - 2 * r, 20], center = true);
      cylinder(1, r = r);
    }
}

// color("#0000ff80") { screen(); }

module holder() {
  difference() {
    translate([-10, -10, 0])
      minkowski() {
        cube([50, 50, 1]);
        cylinder(1, r = 1);
      }
    translate([35, 35, -.01])
      cylinder(20, d=3);
    translate([35, -5, -.01])
      cylinder(20, d=3);
    translate([-5, 35, -.01])
      cylinder(20, d=3);
    translate([-5, -5, -.01])
      cylinder(20, d=3);
    translate([15, 37, -.01])
      cube([14, 10, 10], center = true);
  }
}

module bezel_0() {
  minkowski() {
    cube([30, 30, 1]);
    cylinder(1, r = 1);
  }
}

module bezel_1() {
  translate([0, -1, 0])
    cube([30, 32, 2]);
}

module ribbing() {
  pitch = 1;
  s = pitch / sqrt(2) ;
  for (i = [0:floor(20 / pitch)])
    translate([0, i * pitch, 0])
      rotate([0, 0, 45])
        cube([s, s, 2]);
}

module bezel_2() {
  r = 1;
  union() {
    minkowski() {
      translate([r, 0, 0])
        cube([30 - 2 * r, 32 - 2 * r, 1]);
      cylinder(1, r = 1);
    }
    translate([0, 5, 0])
      ribbing();
    translate([30, 5, 0])
      ribbing();
    // lock edges
    translate([3, -1, 2])
      cube([24, 0.6, 0.5]);
    translate([3, 30.4, 2])
      cube([24, 0.6, 0.5]);
  }
}

module td2() {
  r = 1;
  union() {
    minkowski() {
      translate([r, 0, 0])
        cube([30 - 2 * r, 32 - 2 * r, 1]);
      cylinder(1, r = 1.1);
    }
    translate([0, 5, 0])
      ribbing();
    translate([30, 5, 0])
      ribbing();
  }
}

difference() {
  cube([40, 40, 4]);
  translate([5, 5, -0.01])
    td2();
  translate([5, 5, 1.1])
    td2();
  translate([5, 5, 3])
    td2();
  translate([20, 20, 0])
    cube([20, 50, 6], center = true);
}
cube([5.5, 40, 1]);
translate([34.5, 0, 0])
  cube([5.5, 40, 1]);

translate([20, 20, 0])
rotate([270, 0, 0])
  torus(r1 = 5 / 2, r2 = 35 / 2, angle = 179.99);
translate([20, 20, -35 / 2])
rotate([130, 0, 0]) {
  cylinder(12, r1 = 2.5, r2 = 12);
}
