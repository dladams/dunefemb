{
  int chan = 18;
  DataMap::dbg(1);
  FembTestAnalyzer ftc(121, chan, 2, 2);
  ftc.setTickPeriod(497);
  //ftc.draw("gainh");
  ftc.draw("dev")->hist()->Fit("gaus");
  cout << "For help: ftc.draw()" << endl;
  cout << "For tickmod tree: ftc.tickModTree()" << endl;
}
