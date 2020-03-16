// common variables.
var chrID    = "22";
var nation_1 = "GWD";
var nation_2 = "JPT";
var dirname_1 = "chr" + chrID + "_" + nation_1;
var dirname_2 = "chr" + chrID + "_" + nation_2;


var iteration      = 100;
var regularization = 1;    // 0: no regularization, 1: regularization.
var pos_list = "17885993,17357634,17955257,17875243,18778042,17442951,17655113,17669211,17785372,17178281";
var theta = BiORAM.ML.LogisticRegression(chrID, nation_1, dirname_1, nation_2, dirname_2, pos_list, iteration, regularization);

var reduced = 5;
pos_list = pos_list.SortListFromTheta(theta, reduced);

var n_component = 2;

var GWD_data = BiORAM.ML.PCA(chrID, nation_1, dirname_1, pos_list, n_component);
var GWD_filename = "./client_data/chr" + chrID + "_GWD.data";
BiORAM.fwrite(GWD_filename, GWD_data);

var JPT_data = BiORAM.ML.PCA(chrID, nation_2, dirname_2, pos_list, n_component);
var JPT_filename = "./client_data/chr" + chrID + "_JPT.data";
BiORAM.fwrite(JPT_filename, JPT_data);
