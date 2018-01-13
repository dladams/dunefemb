{
  string fname = "femb_raw_tickmod_femb12_g2_s2_pext.root";
  FembTestTickModViewer tmv(fname);
  cout << "# tree entries: " << tmv.size() << endl;
  cout << "For help: tmv.draw()" << endl;
}
