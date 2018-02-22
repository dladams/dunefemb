{
  string fname = "femb_raw_tickmod_femb12_g2_s2_pext.root";
  //fname = "femb_raw_tickmod_femb3_g99_s99_pint_cint_chan000-031.root";
  fname = "femb_raw_tickmod_femb1_g0_s2_pext.root";
  fname = "femb_calib_tickmod_femb17_g2_s2_pext.root";
  //FembTestTickModViewer tmv(fname, 0, 5);
  FembTestTickModViewer tmv(fname, 0, 128);
  cout << "# tree channels: " << tmv.channels().size() << endl;
  cout << "For help: tmv.draw()" << endl;
  //tmv.tmt().file()->ls();
}
