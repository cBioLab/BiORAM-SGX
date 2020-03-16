// common variables.
var Z_blocks = 6;
var chrID    = "22";
var nation_1 = "GWD";
var nation_2 = "JPT";
var dirname_1 = "chr" + chrID + "_" + nation_1;
var dirname_2 = "chr" + chrID + "_" + nation_2;


var position = 17885993;  // for chr22.
var p_value = BiORAM.ML.Fisher(chrID, nation_1, dirname_1, nation_2, dirname_2, position);
BiORAM.console.log("chrID: " + chrID + ", position: " + position + "(" + nation_1 + ", " + nation_2 + ")\np_value: " + p_value);
