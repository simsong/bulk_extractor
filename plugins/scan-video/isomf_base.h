#ifndef ISOMF_BASE_H
#define ISOMF_BASE_H

namespace isomf {

/// Generic Box (ISO/IEC 14496-12 4.2)
class box 
{
  public:
    box();
    box(const std::string& box_type);
    box(const box& b);
    virtual ~box() {};

    std::string type() const { return _type; }
    void get_extended_type(char* type_buf) const;
    void set_extended_type(char* type_buf);

    virtual size_t content_size() const;
    virtual size_t size() const;
    bool is_small() const;

    box& operator=(const box& box);

    virtual size_t parse(char* buf, size_t sz);
    virtual size_t write(char* buf, size_t sz) const;

    const box* parent() const { return _parent; }
    void parent(const box& p) { _parent = &p; }

    std::list<const box*> children() const { return _children; }
    void add(const box& c);
    void remove(const box& c);

  protected:
    size_t parse_header(char* buf, size_t sz);
    size_t parse_children(char* buf, size_t sz);

    size_t write_header(char* buf, size_t sz) const;
    size_t write_children(char* buf, size_t sz) const;

    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;

    std::string _type;
    char _extended_type[16];

    const box* _parent;
    std::list<const box*> _children;
};

/// Full Box (ISO/IEC 14496-12 4.2)
class full_box : public box
{
  public:
    full_box();
    virtual ~full_box() {};

    unsigned version() const { return _version; }
    void version(unsigned v) { _version = v; }
    unsigned flags() const { return _flags; }
    void flags(unsigned f) { _flags = f; }

    virtual size_t content_size() const 
    { return super::content_size() + 4; }

    full_box& operator=(const full_box& box);

  protected:
    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;

    unsigned _version;
    unsigned _flags;

  private:  
    typedef box super;
};

/// Generic data holder 
class data_holder
{
  public:
    data_holder();
    data_holder(const data_holder& d);
    virtual ~data_holder();

    unsigned size() const { return _data_size; }
    
    void clear();
    size_t set_data(const char* data, size_t size);
    size_t get_data(char* data, size_t size) const;

    virtual size_t parse(const char* buf, size_t sz) { return 0; };
    virtual size_t write(char* buf, size_t sz) const { return 0; };

    data_holder& operator=(const data_holder& d);

  protected:
    char* _data;
    unsigned _data_size;
};

}; // namespace isomf

#endif
