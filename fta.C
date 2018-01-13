{
  int chan = 17;
  FembTestAnalyzer fta(110, chan, 2, 2);
  fta.setTickPeriod(497);
  fta.draw("pedlim");
  //fta.writeCalibFcl();
  cout << "For help: fta.draw()" << endl;
}
