// common variables.
var Z_blocks = 6;
var chrID    = "22";
// var nation  = "GWD";
var nation  = "JPT";
var dirname = "chr" + chrID + "_" + nation;

var chr22_pos_list = "17885993,17357634,17955257,17875243,18778042,17442951,17655113,17669211,17785372,17178281";
var n_component = 2;
var JPT_data = BiORAM.ML.PCA(chrID, nation, dirname, chr22_pos_list, n_component);

BiORAM.console.log(JPT_data);
BiORAM.fwrite("./client_data/sample_PCA_JPT.data", JPT_data);
