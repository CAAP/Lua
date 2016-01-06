
static gdcm::Global &g = gdcm::Global::GetInstance();

static const  gdcm::Defs &defs = g.GetDefs();

static gdcm::Dicts &dicts = g.GetDicts();

int basicAnonymizer( gdcm::File * );

unsigned int nAttributes( );

void getAttributes( gdcm::Tag *ptag);

