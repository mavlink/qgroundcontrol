#ifndef MP_PARSER_MESSAGE_PROVIDER_H
#define MP_PARSER_MESSAGE_PROVIDER_H

#include <vector>
#include <memory>

#include "mpDefines.h"
#include "mpTypes.h"


MUP_NAMESPACE_START

  //-----------------------------------------------------------------------------------------------
  /** \brief Base class for Parser Message providing classes. */
  class ParserMessageProviderBase
  {
  friend class std::unique_ptr<ParserMessageProviderBase>;

  public:
    ParserMessageProviderBase();
    virtual ~ParserMessageProviderBase();

    void Init();
    string_type GetErrorMsg(EErrorCodes errc) const;

  private:
    // Disable CC and assignment operator for this class and derivatives
    ParserMessageProviderBase(const ParserMessageProviderBase &ref);
    ParserMessageProviderBase& operator=(const ParserMessageProviderBase &ref);

  protected:
    std::vector<string_type>  m_vErrMsg;

    virtual void InitErrorMessages() = 0;
  };

  //-----------------------------------------------------------------------------------------------
  /** \brief English versions of parser messages. */
  class ParserMessageProviderEnglish : public ParserMessageProviderBase
  {
  public:
    ParserMessageProviderEnglish();

  protected:
    virtual void InitErrorMessages();
  };

  //-----------------------------------------------------------------------------------------------
  /** \brief German versions of parser messages. */
  class ParserMessageProviderGerman : public ParserMessageProviderBase
  {
  public:
    ParserMessageProviderGerman();

  protected:
    virtual void InitErrorMessages();
  };

MUP_NAMESPACE_END

#endif
