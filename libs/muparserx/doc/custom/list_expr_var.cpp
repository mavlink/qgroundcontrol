void ListExprVar(const mu::ParserBase &parser)
{
  varmap_type variables = parser.GetUsedVar();
  if (!variables.size())
    mu::console() << "Expression does not contain variables\n";
  else
  {
    mu::console() << "Number: " << (int)variables.size() << "\n";
    mu::varmap_type::const_iterator item = variables.begin();

    for (; item!=variables.end(); ++item)
      mu::console() << "Name: " << item->first << "   Address: [0x" << item->second << "]\n";
  }
}

