// common variables.
var Z_blocks = 6;
var chrID    = "22";
var nation_1 = "GWD";
var nation_2 = "JPT";
var in_dirname_1  = "chr" + chrID + "_" + nation_1;
var out_dirname_1 = "chr" + chrID + "_" + nation_1;
var in_dirname_2  = "chr" + chrID + "_" + nation_2;
var out_dirname_2 = "chr" + chrID + "_" + nation_2;

BiORAM.console.log("Create ORAM Tree: chrID = " + chrID + ", nation = " + nation_2 + "(sample).");
// BiORAM.ORAM.init(Z_blocks, in_dirname_1, out_dirname_1);
BiORAM.ORAM.init(Z_blocks, in_dirname_2, out_dirname_2);
BiORAM.console.log("SUCCEEDED: create ORAM Tree...");
