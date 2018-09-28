void ListVar(const mu::ParserBase &parser)
{
  mu::varmap_type variables = parser.GetVar();
  if (!variables.size())
    return;

  cout << "\nParser variables:\n";
  cout <<   "-----------------\n";
  cout << "Number: " << (int)variables.size() << "\n";
  varmap_type::const_iterator item = variables.begin();
  for (; item!=variables.end(); ++item)
    mu::console() << _T("Name: ") << item->first << _T("   Address: [0x") << item->second << _T("]\n");
}
