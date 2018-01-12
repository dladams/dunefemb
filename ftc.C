{
  DataMap::dbg(1);
  FembTestAnalyzer ftc(121, 12, 2, 2);
  ftc.setTickPeriod(497);
  //ftc.draw("gainh");
  ftc.draw("pedlim");
  cout << "For help: ftc.draw()" << endl;
  cout << "For tickmod tree: ftc.tickModTree()" << endl;
}
