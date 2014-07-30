/*
 * Convert Eagle .brd format to gEDA pcb format.
 *
 * Copyright 2014 Brian Davis, all rights reserved.
 */


// Standard C library includes
#include <cstdlib>
#include <cassert>
#include <cstring>

// STL includes
#include <queue>
#include <iostream>
#include <string>
#include <stdexcept>
#include <map>

// Boost includes
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/units/io.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/base_units/us/mil.hpp>

// Xerces includes
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/framework/StdInInputSource.hpp>
#include <xercesc/sax/Locator.hpp>

// Local includes
#include "boost_unit_extras.hpp"

using namespace std;
using namespace xercesc;
namespace po = boost::program_options;
XERCES_CPP_NAMESPACE_USE

namespace jrl
{

static const double INCHES_PER_MM = 0.0393701;

string
getStlString(const XMLCh * const xmlString)
{
  char * transcoded = XMLString::transcode(xmlString);
  string result(transcoded);
  XMLString::release(&transcoded);
  return result;
}

ostream &
operator<<(ostream &target, const XMLCh * const &outgoing)
{
  char * transcoded = XMLString::transcode(outgoing);
  target << transcoded;
  XMLString::release(&transcoded);
  return target;
}

class SAXHandler : public HandlerBase
{
public:

  // Constructors/destructors

  SAXHandler()
    : locator_(NULL), 
      isDefiningLayers_(false), isDefiningBoard_(false),
      isDefiningPlain_(false), isDefiningText_(false),
      isDefiningDescription_(false), isDefiningNote_(false),
      isDefiningLibraries_(false), isDefiningLibrary_(false),
      isDefiningPackages_(false), isDefiningPackage_(false),
      currentText_(NULL), currentPackage_(NULL)
  {
  }

  virtual
  ~SAXHandler(void)
  {
    delete locator_;
  }

  // Member functions

  void
  finalize()
  {
    for (CountIterator entry = elementCounts_.begin();
         entry != elementCounts_.end(); ++entry) {
      cerr << "DBG element " << entry->first << " -> " << entry->second << endl;
    }
    // for (CountIterator entry = layerCounts_.begin();
    //      entry != layerCounts_.end(); ++entry) {
    //   string mapping = layerNames_[entry->first];
    //   cerr << "DBG layer " << mapping << " (" << entry->first << ") -> "
    //        << entry->second << endl;
    // }
  }

  // DocumentHandler overrides

  void
  startDocument()
  {
    cerr << "DBG start of document" << endl;
  }

  void
  endDocument()
  {
    cerr << "DBG end of document" << endl;
  }

  void
  startElement(const XMLCh * const elementName,
               AttributeList &attributes)
  {
    // string name = StrX(elementName).stringForm();
    string name(getStlString(elementName));
    ++elementCounts_[name];
    if (LAYERS == name) {
      // No attributes expected for layers element
      assert(0 == attributes.getLength());
      assert(!isDefiningLayers_);
      assert(!isDefiningBoard_);
      assert(!isDefiningPlain_);
      isDefiningLayers_ = true;
    }
    else if (BOARD == name) {
      assert(!isDefiningLayers_);
      assert(!isDefiningBoard_);
      assert(!isDefiningPlain_);
      isDefiningBoard_ = true;
    }
    else if (PLAIN == name) {
      assert(!isDefiningLayers_);
      assert(isDefiningBoard_);
      assert(!isDefiningPlain_);
      isDefiningPlain_ = true;
    }
    else if (LAYER == name) {
      // NOTE: singular LAYER here instead of plural LAYERS
      assert(isDefiningLayers_);
      assert(!isDefiningBoard_);
      assert(!isDefiningPlain_);
      // handleLayerDefinition(attributes);
    }
    else if (TEXT == name) {
      cerr << "DBG starting text";
      if (NULL != locator_) {
        cerr << " at " << locator_->getLineNumber() << endl;
      }
      else {
        cerr << endl;
      }
      assert(!isDefiningLayers_);
      assert(!isDefiningDescription_);
      // assert(NULL == currentText_);
      currentText_ = new Text;
      isDefiningText_ = true;
      handleTextDefinition(attributes);
    }
    else if (DESCRIPTION == name) {
      assert(!isDefiningLayers_);
      assert(!isDefiningDescription_);
      assert(!isDefiningNote_);
      assert(!isDefiningText_);
      // NOTE: current assumption is that descriptions only apply to
      // packages.
      assert(isDefiningPackages_);
      assert(isDefiningPackage_);
      currentText_ = new Text;
      isDefiningDescription_ = true;
      handleTextDefinition(attributes);
    }
    else if (NOTE == name) {
      assert(!isDefiningLayers_);
      assert(!isDefiningDescription_);
      assert(!isDefiningNote_);
      isDefiningNote_ = true;
    }
    else if (WIRE == name) {
      assert(!isDefiningLayers_);
      // TODO: add asserts
      handleWireDefinition(attributes);
    }
    else if (HOLE == name) {
      assert(!isDefiningLayers_);
      // TODO: add asserts
      handleHoleDefinition(attributes);
    }
    else if (RECTANGLE == name) {
      // TODO: add asserts
      assert(!isDefiningLayers_);
      handleRectangleDefinition(attributes);
    }
    else if (CIRCLE == name) {
      // TODO: add asserts
      assert(!isDefiningLayers_);
      handleCircleDefinition(attributes);
    }
    else if (PACKAGES == name) {
      assert(!isDefiningPackages_);
      assert(!isDefiningPackage_);
      isDefiningPackages_ = true;
    }
    else if (PACKAGE == name) {
      // NOTE: singular PACKAGE here instead of plural PACKAGES
      assert(isDefiningPackages_);
      assert(!isDefiningPackage_);
      isDefiningPackage_ = true;
      currentPackage_ = new Package;
    }
    else {
      ++elementCounts_[name];
    }
  }

  void
  endElement(const XMLCh * const elementName)
  {
    string name = getStlString(elementName);
    if (LAYERS == name) {
      assert(isDefiningLayers_);
      assert(!isDefiningBoard_);
      assert(!isDefiningPlain_);
      isDefiningLayers_ = false;
    }
    else if (BOARD == name) {
      assert(!isDefiningLayers_);
      assert(isDefiningBoard_);
      assert(!isDefiningPlain_);
      isDefiningBoard_ = false;
    }
    else if (PLAIN == name) {
      assert(!isDefiningLayers_);
      assert(isDefiningBoard_);
      assert(isDefiningPlain_);
      isDefiningPlain_ = false;
    }
    else if (TEXT == name) {
      cerr << "DBG ending text" << endl;
      assert(!isDefiningLayers_);
      assert(currentText_);
      // TODO: are the following correct?
      // assert(isDefiningPlain_);
      // isDefiningPlain_ = false;
      assert(isDefiningText_);
      if (isDefiningPackage_) {
        assert(NULL != currentPackage_);
        currentPackage_->addText(currentText_);
      }
      else {
        assert(!isDefiningPackages_);
        assert(NULL == currentPackage_);
        board_.addText(currentText_);
      }
      isDefiningText_ = false;
      // cerr << "DBG text x=" << currentText_->x
      //      << ",y=" << currentText_->y
      //      << ",size=" << currentText_->size
      //      << ",layer=" << currentText_->layer
      //      << ",ratio=" << currentText_->ratio
      //      << ",rotation=" << currentText_->rotationDegrees
      //      << ",value='" << currentText_->value << "'" << endl;
      currentText_ = NULL;
    }
    else if (DESCRIPTION == name) {
      assert(!isDefiningText_);
      assert(isDefiningDescription_);
      assert(!isDefiningNote_);
      // NOTE: current assumption is that descriptions only apply to
      // packages.
      assert(isDefiningPackages_);
      assert(isDefiningPackage_);
      assert(NULL != currentPackage_);
      currentPackage_->setDescription(*currentText_);
      // TODO: memory leak here
      delete currentText_;
      isDefiningDescription_ = false;
    }
    else if (NOTE == name) {
      assert(!isDefiningText_);
      assert(!isDefiningDescription_);
      assert(isDefiningNote_);
      isDefiningNote_ = false;
    }
    else if (PACKAGE == name) {
      assert(isDefiningPackages_);
      assert(isDefiningPackage_);
      assert(NULL != currentPackage_);
      isDefiningPackage_ = false;
      packages_.push(currentPackage_);
      currentPackage_ = NULL;
    }
    else if (PACKAGES == name) {
      assert(isDefiningPackages_);
      assert(!isDefiningPackage_);
      assert(NULL == currentPackage_);
      isDefiningPackages_ = false;
    }
  }

  void
  characters(const XMLCh * const chars,
             const XMLSize_t length)
  {
    if (isDefiningText_) {
      currentText_->handleCharacters(chars, length);
    }
    else if (isDefiningDescription_) {
      // TODO: reassess
      assert(!isDefiningText_);
      // currentPackage_->handleCharacters(chars, length);
      currentText_->handleCharacters(chars, length);
    }
    else if (isDefiningNote_) {
      // Do nothing
    }
    else {
      cerr << "WARN " << length << " unexpected characters: '"
           << chars << "'" << endl;
    }
  }

  void
  ignorableWhitespace(const XMLCh * const chars,
                      const XMLSize_t length)
  {
    // Unused
  }

  void
  processingInstruction(const XMLCh * const target,
                        const XMLCh * const data)
  {
    cerr << "DBG processing instruction" << endl;
  }

  // ErrorHandler overrides
  void
  warning(const SAXParseException &exc)
  {
    dumpExceptionDetails("WARN", exc);
  }

  void
  error(const SAXParseException &exc)
  {
    dumpExceptionDetails("ERR", exc);
  }

  void
  fatalError(const SAXParseException &exc)
  {
    dumpExceptionDetails("FATAL", exc);
  }

  // DTDHandler interface
  void
  notationDecl(const XMLCh * const name,
               const XMLCh * const publicId,
               const XMLCh * const systemId)
  {
    // Unused
  }

  void
  unparsedEntityDecl(const XMLCh * const name,
                     const XMLCh * const publicId,
                     const XMLCh * const systemId,
                     const XMLCh * const notationName)
  {
    // Unused
  }

  virtual void
  setDocumentLocator(const Locator * const locator)
  {
    if (NULL != locator_) {
      delete locator_;
    }
    locator_ = new LocatorManager(locator);
  }

private:

  // Types

  enum Language {
    ENGLISH,
    GERMAN
  };

  typedef map<string, unsigned> CountMap;
  typedef map<string, unsigned>::iterator CountIterator;
  typedef map<string, string> StringMap;
  typedef map<string, string>::iterator StringIterator;

  /**
   * Wrapper which allows the SAXParser to receive a copy of the
   * Locator object.
   */
  class LocatorManager
  {
  public:
    LocatorManager(const Locator * const locator)
      : locator_(locator)
    {
    }

    const XMLCh *
    getPublicId () const
    {
      return locator_->getPublicId();
    }

    const XMLCh *
    getSystemId () const
    {
      return locator_->getSystemId();
    }

    XMLSSize_t
    getLineNumber () const
    {
      return locator_->getLineNumber();
    }

    XMLSSize_t
    getColumnNumber () const
    {
      return locator_->getColumnNumber();
    }

  private:
    const Locator * const locator_;
  };

  /**
   * Extract a string from an attribute using a policy.
   */
  template <typename GetPolicy> static string
  extract(const AttributeList &attributes,
          const unsigned index)
  {
    return GetPolicy::get(attributes, index);
  }

  /**
   * Policy class for extracting the name of an attribute.
   */
  class NameGetter
  {
  public:
    static string
    get(const AttributeList &attributes,
        const unsigned index)
    {
      return getStlString(attributes.getName(index));
    }
  };

  /**
   * Policy class for extracting the value of an attribute.
   */
  class ValueGetter
  {
  public:
    static string
    get(const AttributeList &attributes,
        const unsigned index)
    {
      return getStlString(attributes.getValue(index));
    }
  };

  /**
   * Xerces SAX parsing API doesn't support an object associated with
   * a single (name, value) attribute, so it is implemented here.
   */
  class Attribute : public pair<const string, const string>
  {
  public:

    // Constructors/destructors

    Attribute(const AttributeList &attributes,
              const unsigned index)
      : attribute_(make_pair(extract<NameGetter>(attributes, index),
                             extract<ValueGetter>(attributes, index)))
    {
    }

    // Member functions

    const string &
    getName() const
    {
      return attribute_.first;
    }

    const string &
    getValue() const
    {
      return attribute_.second;
    }

  private:

    // Data members
    const pair<const string, const string> attribute_;
  };

  /**
   * Mixin for Eagle board file elements which have a specified width.
   */
  class HasWidth
  {
  public:

    // Constructors/destructors

    HasWidth()
      : width_(0.0), hasWidth_(false)
    {}

    // Member functions

    double
    getWidth() const
    {
      assert(hasWidth_);
      return width_;
    }

    bool
    hasWidth() const
    {
      return hasWidth_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      const string &name = attribute.getName();
      const char * const value = attribute.getValue().c_str();
      if (WIDTH == name) {
        assert(!hasWidth_);
        width_ = atof(value);
        hasWidth_ = true;
        return true;
      }
      return false;
    }

  private:

    // Data members

    double width_;
    bool hasWidth_;
  };

  /**
   * Mixin to add layer data to Eagle board file elements.
   */
  class InLayer
  {
  public:

    // Constructors/destructors

    InLayer()
      : layer_(0), hasLayer_(false)
    {}

    // Member functions

    unsigned
    getLayer() const
    {
      assert(hasLayer_);
      return layer_;
    }

    bool
    hasLayer() const
    {
      return hasLayer_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if (LAYER == attribute.getName()) {
        assert(!hasLayer_);
        layer_ = atoi(attribute.getValue().c_str());
        hasLayer_ = true;
        return true;
      }
      return false;
    }

  private:

    // Data members

    unsigned layer_;
    bool hasLayer_;
  };

  // TODO: use boost::units?
  /**
   * Mixin representing the pose (in a robotics/kinematics sense of
   * including both position and rotation) of an element in the Eagle
   * layout.
   */
  class Pose : public InLayer
  {
  public:

    // Constructors/destructors

    Pose()
      : x_(0.0), y_(0.0), rotationDegrees_(0.0),
        hasX_(false), hasY_(false), hasRotation_(false)
    {
    }

    // Member functions

    Millimeters
    getX() const
    {
      assert(hasX_);
      return x_;
    }

    double
    getY() const
    {
      assert(hasY_);
      return y_;
    }

    double
    getRotationDegrees() const
    {
      assert(hasRotation_);
      return rotationDegrees_;
    }

    bool
    hasX() const
    {
      return hasX_;
    }

    bool
    hasY() const
    {
      return hasY_;
    }

    bool
    hasRotation() const
    {
      return hasRotation_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if (!InLayer::tryHandleAttribute(attribute)) {
        const string &name = attribute.getName();
        const char * const value = attribute.getValue().c_str();
        if (X == name) {
          assert(!hasX_);
          x_ = atof(value);
          hasX_ = true;
          return true;
        }
        if (Y == name) {
          assert(!hasY_);
          y_ = atof(value);
          hasY_ = true;
          return true;
        }
        if (ROTATION == name) {
          assert(!hasRotation_);
          rotationDegrees_ = atof(value);
          hasRotation_ = true;
          return true;
        }
        return false;
        
      }
      // Attribute actually specified the layer.
      return true;
    }
  private:

    // Data members

    Millimeters x_;
    double y_;
    double rotationDegrees_;
    bool hasX_;
    bool hasY_;
    bool hasRotation_;
  };

  /**
   * Representation of a text element of an Eagle board or
   * package.
   */
  class Text : public Pose
  {
  public:

    // Constructors/destructors

    Text()
      : language_(ENGLISH), size_(0), ratio_(0), string_(""),
        hasSize_(false), hasRatio_(false), hasString_(false)
    {
    }


    // Member functions

    Language
    getLanguage() const
    {
      return language_;
    }

    double
    getSize() const
    {
      assert(hasSize_);
      return size_;
    }

    double
    getRatio() const
    {
      assert(hasRatio_);
      return ratio_;
    }

    const string &
    getString() const
    {
      assert(hasString_);
      return string_;
    }

    bool
    hasSize() const
    {
      return hasSize_;
    }

    bool
    hasRatio() const
    {
      return hasRatio_;
    }

    bool
    hasString() const
    {
      return hasString_;
    }

    void
    handleCharacters(const XMLCh * const chars,
                     const XMLSize_t length)
    {
      assert(!hasString_);
      string_ = getStlString(chars);
      hasString_ = true;
      // TODO: assert(length == string_.length()); ??
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if (!Pose::tryHandleAttribute(attribute)) {
        const string &name = attribute.getName();
        const char * const value = attribute.getValue().c_str();
        if (SIZE == name) {
          assert(!hasSize_);
          size_ = atof(value);
          hasSize_ = true;
          return true;
        }
        if (RATIO == name) {
          assert(!hasRatio_);
          ratio_ = atof(value);
          hasRatio_ = true;
          return true;
        }
        if (LANGUAGE == name) {
          if (EN == value) {
            language_ = ENGLISH;
          }
          else if (DE == value) {
            language_ = GERMAN;
          }
          else {
            cerr << "WARN unknown language type '" << value << "'" << endl;
          }
          return true;
        }
        if ((FONT == name) || (ALIGN == name)) {
          // TODO: handle this if possible, currently short-circuits
          // processing of this attribute so that it won't be reported
          // as unexpected.
          return true;
        }
        return false;
      }
      return true;
    }

  private:

    // Data members

    Language language_;
    double size_;
    double ratio_;
    string string_;
    bool hasSize_;
    bool hasRatio_;
    bool hasString_;
  };

  /**
   * Representation of a hole element (including a via) of an Eagle
   * board or package.
   *
   * NOTE: doesn't really need the rotation element, but no need to
   * create a separate class with position but no rotation to handle
   * only this case.
   */
  class Hole : public Pose
  {
  public:
    Hole(const bool isVia)
      : drill_(0.0), hasDrill_(false), isVia_(isVia)
    {
    }

    double
    getDrill() const
    {
      assert(hasDrill_);
      return drill_;
    }

    bool
    hasDrill() const
    {
      return hasDrill_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if (!Pose::tryHandleAttribute(attribute)) {
        const string &name = attribute.getName();
        const char * const value = attribute.getValue().c_str();
        if (DRILL == name) {
          assert(!hasDrill_);
          drill_ = atof(value);
          hasDrill_ = true;
          return true;
        }
        return false;
      }
      return true;
    }

  private:
    double drill_;
    bool hasDrill_;
    const bool isVia_;
  };

  /**
   * Mixin to add end point data to certain types of Eagle board file
   * elements.
   */
  class EndPoints : public InLayer, HasWidth
  {
  public:

    // Constructors/destructors

    EndPoints()
      : x1_(0.0), y1_(0.0), x2_(0.0), y2_(0.0),
        hasX1_(false), hasY1_(false), hasX2_(false), hasY2_(false)
    {
    }

    // Member functions

    double
    getX1() const
    {
      assert(hasX1_);
      return x1_;
    }

    double
    getY1() const
    {
      assert(hasY1_);
      return y1_;
    }

    double
    getX2() const
    {
      assert(hasX2_);
      return x2_;
    }

    double
    getY2() const
    {
      assert(hasY2_);
      return y2_;
    }

    bool
    hasX1() const
    {
      return hasX1_;
    }

    bool
    hasY1() const
    {
      return hasY1_;
    }

    bool
    hasX2() const
    {
      return hasX2_;
    }

    bool
    hasY2() const
    {
      return hasY2_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if ((!InLayer::tryHandleAttribute(attribute)) &&
          (!HasWidth::tryHandleAttribute(attribute))) {
        const string &name = attribute.getName();
        const char * const value = attribute.getValue().c_str();
        if (X1 == name) {
          assert(!hasX1_);
          x1_ = atof(value);
          hasX1_ = true;
          return true;
        }
        if (Y1 == name) {
          assert(!hasY1_);
          y1_ = atof(value);
          hasY1_ = true;
          return true;
        }
        if (X2 == name) {
          assert(!hasX2_);
          x2_ = atof(value);
          hasX2_ = true;
          return true;
        }
        if (Y2 == name) {
          assert(!hasY2_);
          y2_ = atof(value);
          hasY2_ = true;
          return true;
        }
        return false;
      }
      return true;
    }

  private:

    // Data members

    double x1_;
    double x2_;
    double y1_;
    double y2_;
    bool hasX1_;
    bool hasY1_;
    bool hasX2_;
    bool hasY2_;
  };

  /**
   * Representation of a wire element of an Eagle board or
   * package.
   */
  class Wire : public EndPoints
  {
  public:

    // Constructors/destructors

    Wire()
      : curve_(0.0), hasCurve_(false)
    {
    }

    // Member functions

    double
    getCurve() const
    {
      return curve_;
    }

    bool
    hasCurve() const
    {
      return hasCurve_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if (!EndPoints::tryHandleAttribute(attribute)) {
        const string &name = attribute.getName();
        const char * const value = attribute.getValue().c_str();
        if (CURVE == name) {
          assert(!hasCurve_);
          curve_ = atof(value);
          hasCurve_ = true;
          return true;
        }
        return false;
      }
      return true;
    }

  private:

    // Data members

    double curve_;
    bool hasCurve_;
  };

  /**
   * Representation of a rectangle element of an Eagle board or
   * package.
   */
  class Rectangle : public EndPoints
  {
  public:

    // Constructors/destructors

    Rectangle()
      : rotationDegrees_(0.0), hasRotation_(false)
    {
    }

    // Member functions

    double
    getRotationDegrees() const
    {
      assert(hasRotation_);
      return rotationDegrees_;
    }

    bool
    hasRotation() const
    {
      return hasRotation_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if (!EndPoints::tryHandleAttribute(attribute)) {
        const string &name = attribute.getName();
        const char * const value = attribute.getValue().c_str();
        if (ROTATION == name) {
          assert(!hasRotation_);
          rotationDegrees_ = atof(value);
          hasRotation_ = true;
          return true;
        }
        return false;
      }
      return true;
    }

  private:

    // Data members

    double rotationDegrees_;
    bool hasRotation_;
  };

  /**
   * Representation of a circle element of an Eagle board or package.
   */
  class Circle : public Pose, HasWidth
  {
  public:

    // Constructors/destructors

    Circle()
      : radius_(0.0), hasRadius_(false)
    {
    }

    // Member functions

    double
    getRadius() const
    {
      assert(hasRadius_);
      return radius_;
    }

    bool
    hasRadius() const
    {
      return hasRadius_;
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      if ((!Pose::tryHandleAttribute(attribute)) &&
          (!HasWidth::tryHandleAttribute(attribute))) {
        const string &name = attribute.getName();
        const char * const value = attribute.getValue().c_str();
        if (RADIUS == name) {
          assert(!hasRadius_);
          radius_ = atof(value);
          hasRadius_ = true;
          return true;
        }
        return false;
      }
      return true;
    }

  private:

    // Data members

    double radius_;
    bool hasRadius_;
  };

  /**
   * Representation of an Eagle board or package.
   */
  class Board
  {
  public:

    // Member functions

#define ADD_OBJECT(otype, oname) \
    void \
    add##otype(otype *oname) \
    { \
      oname##Objects_.push(oname); \
    }

    ADD_OBJECT(Text, text);
    ADD_OBJECT(Hole, hole);
    ADD_OBJECT(Wire, wire);
    ADD_OBJECT(Circle, circle);
    ADD_OBJECT(Rectangle, rectangle);
    // void
    // addText(Text *text)
    // {
    //   textObjects_.push(text);
    // }

    // void
    // addHole(Hole *hole)
    // {
    //   holeObjects_.push(hole);
    // }

    // void
    // addWire(Wire *wire)
    // {
    //   wireObjects_.push(wire);
    // }

    // void
    // addCircle(Circle *circle)
    // {
    //   circleObjects_.push(circle);
    // }

#undef ADD_OBJECT

  private:

    // Data members

    queue<Text *> textObjects_;
    queue<Hole *> holeObjects_;
    queue<Wire *> wireObjects_;
    queue<Circle *> circleObjects_;
    queue<Rectangle *> rectangleObjects_;
  };

  class Package : public Board
  {
  public:

    // Constructors/destructors

    Package()
      : name_(""), description_(""),
        hasName_(false), hasDescription_(false)
    {
    }

    // Member functions

    string
    getName() const
    {
      assert(hasName_);
      return name_;
    }

    bool
    hasName() const
    {
      return hasName_;
    }

    // void
    // handleCharacters(const XMLCh * const chars,
    //                  const XMLSize_t length)
    // {
    //   cerr << "DBG handling characters in package" << endl;
    //   // assert(!hasDescription_);
    //   description_ = getStlString(chars);
    //   hasDescription_ = true;
    //   // TODO: assert(length == description_.length()); ??
    // }

    void
    setDescription(const Text &description)
    {
      assert(description.hasString());
      if (ENGLISH == description.getLanguage()) {
        description_ = description.getString();
      }
    }

    bool
    tryHandleAttribute(const Attribute &attribute)
    {
      // TODO: error checking on conversions
      const string &name = attribute.getName();
      const char * const value = attribute.getValue().c_str();
      if (NAME == name) {
        assert(!hasName_);
        name_ = name;
        hasName_ = true;
        return true;
      }
      else if (DESCRIPTION == name) {
        // TODO: ignore?
      }
      return false;
    }

  private:

    // Data members

    string name_;
    string description_;
    bool hasName_;
    bool hasDescription_;
  };

  // Member functions

  void
  dumpExceptionDetails(const char *errorType,
                       const SAXParseException &exc)
  {
    // cerr << errorType << " in file " << StrX(exc.getSystemId()) << ", line "
    //      << exc.getLineNumber() << ", char " << exc.getColumnNumber()
    //      << ": " << StrX(exc.getMessage()) << endl;
    cerr << errorType << " in file " << exc.getSystemId() << ", line "
         << exc.getLineNumber() << ", char " << exc.getColumnNumber()
         << ": " << exc.getMessage() << endl;
  }

  // TODO: is it necessary to have a list of all layer definitions, or
  // will a fixed mapping from Eagle layers to pcb layers suffice?
  //
  // void
  // handleLayerDefinition(AttributeList &attributes)
  // {
  //   bool isActive = false;
  //   const unsigned count = attributes.getLength();
  //   for (unsigned index = 0; index < count; ++index) {
  //     Attribute attribute(attributes, index);
  //     // TODO: assert that CDATA or ENUMERATION are the only types?
      
  //     if ((ACTIVE == attribute.getName()) && (YES == attribute.getValue())) {
  //       isActive = true;
  //     }
  //     else if (NUMBER == attribute.getName()) {
  //       layerNumber = attribute.getValue();
  //     }
  //     else if (NAME == attribute.getName()) {
  //       layerName = attribute.getValue();
  //     }
  //   }
  //   if (isActive) {
  //     layerNames_[layerNumber] = attribute.getName();
  //   }
  // }

  void
  handleTextDefinition(AttributeList &attributes)
  {
    assert(NULL != currentText_);
    const unsigned count = attributes.getLength();
    for (unsigned index = 0; index < count; ++index) {
      Attribute attribute(attributes, index);
      if (currentText_->tryHandleAttribute(attribute)) {
        continue;
      }
      else if ((FONT == attribute.getName()) ||
               (ALIGN == attribute.getName())) {
        // TODO: handle this if possible
        continue;
      }
      else {
        cerr << "WARN unexpected attribute '" << attribute.getName()
             << "' in text definition" << endl;
      }
    }
    // NOTE: wait until the end of the text definition to save the
    // text value since the characters are not defined along with the
    // other attributes.
  }

  void
  handleWireDefinition(AttributeList &attributes)
  {
    Wire *wire = new Wire;
    const unsigned count = attributes.getLength();
    for (unsigned index = 0; index < count; ++index) {
      Attribute attribute(attributes, index);
      if (!wire->tryHandleAttribute(attribute)) {
        if (CAP == attribute.getName()) {
          // Handle 'cap' attribute, the only instances observed have the
          // value 'flat' and 'round'
          // cerr << "WARN ignoring 'cap' value '" << value << "' in wire "
          //      << "definition" << endl;
        }
        else if (STYLE == attribute.getName()) {
          // NOTE: Possibly specific to rendering of wires; the only
          // instance observed has the value 'shortdash'
        }
        else {
          cerr << "WARN unexpected attribute '" << attribute.getName()
               << "' in wire definition" << endl;
        }
      }
    }
    if (isDefiningPackage_) {
      assert(isDefiningPackages_);
      assert(NULL != currentPackage_);
      currentPackage_->addWire(wire);
    }
    else {
      assert(!isDefiningPackages_);
      assert(NULL == currentPackage_);
      board_.addWire(wire);
    }
  }

  void
  handleHoleDefinition(AttributeList &attributes)
  {
    Hole *hole = new Hole(false);  // Not a Via
    const unsigned count = attributes.getLength();
    for (unsigned index = 0; index < count; ++index) {
      Attribute attribute(attributes, index);
      if (!hole->tryHandleAttribute(attribute)) {
        cerr << "WARN unexpected attribute '" << attribute.getName()
             << "' in hole definition" << endl;
      }
    }
    if (isDefiningPackage_) {
      assert(NULL != currentPackage_);
      currentPackage_->addHole(hole);
    }
    else {
      assert(!isDefiningPackages_);
      assert(NULL == currentPackage_);
      board_.addHole(hole);
    }
  }

  void
  handleRectangleDefinition(AttributeList &attributes)
  {
    // NOTE: attributes of a rectangle are the same as a wire, but it
    // needs to go on a different list.
    Rectangle *rectangle = new Rectangle;
    const unsigned count = attributes.getLength();
    for (unsigned index = 0; index < count; ++index) {
      Attribute attribute(attributes, index);
      if (!rectangle->tryHandleAttribute(attribute)) {
        cerr << "WARN unexpected attribute '" << attribute.getName()
             << "' in rectangle definition" << endl;
      }
    }
    if (isDefiningPackage_) {
      assert(NULL != currentPackage_);
      currentPackage_->addRectangle(rectangle);
    }
    else {
      assert(!isDefiningPackages_);
      assert(NULL == currentPackage_);
      board_.addRectangle(rectangle);
    }
  }

  void
  handleCircleDefinition(AttributeList &attributes)
  {
    Circle *circle = new Circle;
    const unsigned count = attributes.getLength();
    for (unsigned index = 0; index < count; ++index) {
      Attribute attribute(attributes, index);
      if (!circle->tryHandleAttribute(attribute)) {
        cerr << "WARN unexpected attribute '" << attribute.getName()
             << "' in circle definition" << endl;
      }
    }
  }

  // Constants

  // attribute types
  static const string CDATA;
  static const string ENUMERATION;
  // Eagle element and attribute names
  static const string LAYERS;
  static const string LAYER;
  static const string NUMBER;
  static const string NAME;
  static const string ACTIVE;
  static const string YES;
  static const string NO;
  static const string BOARD;
  static const string PLAIN;
  static const string TEXT;
  static const string WIRE;
  static const string HOLE;
  static const string RECTANGLE;
  static const string CIRCLE;
  static const string X;
  static const string Y;
  static const string X1;
  static const string Y1;
  static const string X2;
  static const string Y2;
  static const string SIZE;
  static const string RATIO;
  static const string ROTATION;
  static const string FONT;
  static const string ALIGN;
  static const string DESCRIPTION;
  static const string LANGUAGE;
  static const string EN;
  static const string DE;
  static const string NOTE;
  static const string WIDTH;
  static const string CURVE;
  static const string DRILL;
  static const string CAP;
  static const string STYLE;
  static const string RADIUS;
  static const string LIBRARIES;
  static const string LIBRARY;
  static const string PACKAGES;
  static const string PACKAGE;
  static const string SMD;
  static const string PAD;

  // Data members

  LocatorManager *locator_;

  CountMap elementCounts_;
  // CountMap layerCounts_;
  StringMap layerNames_;

  Board board_;
  queue<Package *> packages_;

  // TODO: convert flags into a state machine
  bool isDefiningLayers_;
  bool isDefiningBoard_;
  bool isDefiningPlain_;
  bool isDefiningText_;
  bool isDefiningDescription_;
  bool isDefiningNote_;
  bool isDefiningLibraries_;
  bool isDefiningLibrary_;
  bool isDefiningPackages_;
  bool isDefiningPackage_;

  // Current variables used when definitions cross multiple elements.
  Text *currentText_;
  Package *currentPackage_;
};

const string SAXHandler::CDATA = "CDATA";
const string SAXHandler::ENUMERATION = "ENUMERATION";
const string SAXHandler::LAYERS = "layers";
const string SAXHandler::LAYER = "layer";
const string SAXHandler::NUMBER = "number";
const string SAXHandler::NAME = "name";
const string SAXHandler::ACTIVE = "active";
const string SAXHandler::YES = "yes";
const string SAXHandler::NO = "no";
const string SAXHandler::BOARD = "board";
const string SAXHandler::PLAIN = "plain";
const string SAXHandler::TEXT = "text";
const string SAXHandler::WIRE = "wire";
const string SAXHandler::HOLE = "hole";
const string SAXHandler::RECTANGLE = "rectangle";
const string SAXHandler::CIRCLE = "circle";
const string SAXHandler::X = "x";
const string SAXHandler::Y = "y";
const string SAXHandler::X1 = "x1";
const string SAXHandler::Y1 = "y1";
const string SAXHandler::X2 = "x2";
const string SAXHandler::Y2 = "y2";
const string SAXHandler::SIZE = "size";
const string SAXHandler::RATIO = "ratio";
const string SAXHandler::ROTATION = "rot";
const string SAXHandler::FONT = "font";
const string SAXHandler::ALIGN = "align";
const string SAXHandler::DESCRIPTION = "description";
const string SAXHandler::LANGUAGE = "language";
const string SAXHandler::EN = "en";  // English
const string SAXHandler::DE = "de";  // German
const string SAXHandler::NOTE = "note";
const string SAXHandler::WIDTH = "width";
const string SAXHandler::CURVE = "curve";
const string SAXHandler::DRILL = "drill";
const string SAXHandler::CAP = "cap";
const string SAXHandler::STYLE = "style";
const string SAXHandler::RADIUS = "radius";
const string SAXHandler::LIBRARIES = "libraries";
const string SAXHandler::LIBRARY = "library";
const string SAXHandler::PACKAGES = "packages";
const string SAXHandler::PACKAGE = "package";
const string SAXHandler::SMD = "smd";
const string SAXHandler::PAD = "pad";
};

// Constant values for gEDA pcb output file, all comments are taken
// directly from the pcb manual.

// File format version. This version number represents the date when
// the pcb file format was last changed.  Any version of pcb build
// from sources equal to or newer than this number should be able to
// read the file. If this line is not present in the input file then
// file format compatibility is not checked.
static const char *FILE_VERSION = "FileVersion[20070407]";
// Relative size of thermal fingers. A value of 1.0 makes the finger
// width twice the clearance gap width (measured across the gap, not
// diameter). The normal value is 0.5, which results in a finger width
// the same as the clearance gap width.
static const char *THERMAL_GAP = "Thermal[0.500000]";
// NOTE: these symbolic values were deduced from an extant file using
// the flags documentation.
//
// nameonpcb - Display names of elements, instead of refdes. ??
//
// uniquename - Force unique names on board.
//
// clearnew - New lines/arc clear polygons.
//
// snappin - Crosshair snaps to pins and pads.
static const char *LAYOUT_FLAGS = "Flags(\"nameonpcb,uniquename,clearnew,snappin\")";
// Encodes the layer grouping information. Each group is separated by
// a colon, each member of each group is separated by a comma. Group
// members are either numbers from 1..N for each layer, and the
// letters c or s representing the component side and solder side of
// the board. Including c or s marks that group as being the top or
// bottom side of the board.
//
// Default here has group 1 on the component side and group 6 on the
// solder side
static const char *LAYOUT_GROUPS = "Groups(\"1,c:2:3:4:5:6,s:7:8\")";

using namespace jrl;

int
main(const int argc, const char *argv[])
{
  // Argument processing
  po::variables_map args;
  {
    po::options_description description("Usage (input file on stdio, output to stdout)");
    description.add_options()
      ("help,h", "Display usage");

    po::store(po::command_line_parser(argc, argv).options(description).run(), args);
    po::notify(args);

    if (args.count("help")){
      cerr << description;
      return -1;
    }
  }

  bool doTerminate = false;
  int result = 0;
  SAXParser *parser = NULL;

  try {
    XMLPlatformUtils::Initialize();
    doTerminate = true;

    parser = new SAXParser;
    // NOTE: SAXParser configuration stolen from SAXPrint.cpp sample code.
    parser->setValidationScheme(SAXParser::Val_Auto);
    parser->setDoNamespaces(false);
    parser->setDoSchema(false);
    parser->setHandleMultipleImports(true);
    parser->setValidationSchemaFullChecking(false);

    SAXHandler handler;
    parser->setDocumentHandler(&handler);
    parser->setErrorHandler(&handler);
    parser->parse(StdInInputSource());
    cerr << "Parsing complete with " << parser->getErrorCount() << " errors" << endl;

    cout << "# Output generated from Eagle .brd file automatically by "
         << "eagle2gedapcb." << endl << endl;
    cout << FILE_VERSION << endl;
    // TODO: PCB element
    //
    // name should probably be set by command line argument.
    //
    // dimensions should be command line argument or calculation based
    // on ensuring that all components fit.

    // NOTE: not including the following optional layout file elements:
    // Grid (TODO: this can be determined from Eagle file)
    // Cursor
    // Styles
    // Symbols
    // Netlists
    cout << LAYOUT_FLAGS << endl;
    cout << LAYOUT_GROUPS << endl;
    // TODO: set the PCB::grid::unit attribute based on the grid
    // setting from the Eagle file
    //
    // Eagle example:
    // <grid distance="25" unitdist="mil" unit="mil" style="dots"
    //       multiple="8" display="no" altdistance="6.25" altunitdist="mil"
    //       altunit="mil"/>
    //
    // pcb example
    // Attribute("PCB::grid::unit" "mil")
    handler.finalize();
  }
  catch (const OutOfMemoryException &) {
    cerr << "FATAL out of memory exception at top level" << endl;
    result = -2;
  }
  catch (const XMLException &exc) {
    cerr << "FATAL XML exception at top level: "
         << getStlString(exc.getMessage()) << endl;
    result = -2;
  }
  catch (...) {
    cerr << "FATAL unknown/unexpected exception type at top level" << endl;
    result = -2;
  }

  if (NULL != parser) {
    delete parser;
  }
  if (doTerminate) {
    XMLPlatformUtils::Terminate();
  }

  return result;
}
