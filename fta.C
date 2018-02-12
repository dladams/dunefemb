{
  int ifmb = 17;
  FembTestAnalyzer fta(110, ifmb, 2, 2);
  fta.setTickPeriod(497);
  fta.draw("pedlim");
  //fta.writeCalibFcl();
  cout << "For help: fta.draw()" << endl;
}
