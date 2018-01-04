//void test() {
{
  int ifmb = 20;
  cout << "FEMB " << ifmb << endl;
  int opt = 121;    // tickmod ROIs
  opt = 111;        // peak ROIs
  FembTestAnalyzer ftc(opt, ifmb, 2, 2, "", true);
  ftc.setTickPeriod(497);
  ftc.processAll();
  TTree* ptree = ftc.tickModTree()->tree();
  FembTestTickModViewer tmv(*ftc.tickModTree());
  //ftc20.dbg=4;
  //ftc20.processChannelEvent(8,0);
  //ftc20.processChannelEvent(8,4);
  //ftc20.processChannel(8);
/*
  FembTestAnalyzer fta3(17,3,3,"",true,false);
  fta3.processAll();
  fta3.draw("pedch")->draw();
  fta3.draw("pedlimch")->draw();
  //fta3.draw("pedlimch");
  //fta3.draw("ped", 0);
  TPadManipulator man2;
  man2.split(2);
  *man2.man(0) = *fta3.draw("ped", 77);
  *man2.man(1) = *fta3.draw("ped", 78);
  *man2.man(2) = *fta3.draw("ped", 79);
  *man2.man(3) = *fta3.draw("ped", 80);
  man2.draw();
*/
}
