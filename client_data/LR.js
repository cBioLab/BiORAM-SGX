// common variables.
var Z_blocks = 6;
var chrID    = "22";
var nation_1 = "GWD";
var nation_2 = "JPT";
var dirname_1 = "chr" + chrID + "_" + nation_1;
var dirname_2 = "chr" + chrID + "_" + nation_2;



var iteration      = 100;
var regularization = 1;    // 0: no regularization, 1: regularization.
var chr22_pos_list = "17885993,17357634,17955257,17875243,18778042,17442951,17655113,17669211,17785372,17178281";
var theta = BiORAM.ML.LogisticRegression(chrID, nation_1, dirname_1, nation_2, dirname_2, chr22_pos_list, iteration, regularization);

BiORAM.console.log(theta);
BiORAM.fwrite("./client_data/sample_LR.data", theta);
