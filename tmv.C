{
  string fname = "femb_raw_tickmod_femb12_g2_s2_pext.root";
  //fname = "femb_raw_tickmod_femb3_g99_s99_pint_cint_chan000-031.root";
  FembTestTickModViewer tmv(fname);
  cout << "# tree entries: " << tmv.size() << endl;
  cout << "For help: tmv.draw()" << endl;
}
