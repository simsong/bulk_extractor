"""ttable.py:
Module for typesetting tables in ASCII, LaTeX, and HTML.  Perhaps even CSV!
Also creates LaTeX variables.

Good options for AJAX tables are:
http://joomlicious.com/mootable/index.php
* JSON table

http://www.millstream.com.au/view/code/tablekit/
http://www.millstream.com.au/upload/code/tablekit/
 * Real tables
 * MIT license
 * Uses prototype framework
  
http://www.phatfusion.net/sortabletable/
 * MIT License
 * Table is a real table
 * Requires mootools, a compact javascript framework - http://www.mootools.net/

http://dhtmlx.com/docs/products/dhtmlxGrid/
 * Free and "Commercial" version:
 * Table represented as JSON array


Commercial and GPL:
http://www.extjs.com/playpen/ext-2.0/examples/grid/grid3.html
"""

def isnumber(v):
    try:
        return v==0 or v!=0
    except TypeError:
        return False

def tvar(name,value,desc):
    """Create a variable NAME with a given VALUE.
    Primarily for output to LaTeX.
    Normally outputs to stdout, but set tvar.out to be a file handle to go elsewhere."""
    if not hasattr(tvar,"out"):
        import sys;
        tvar.out=sys.stdout
    print("%s: %s " % (desc, value))
    tvar.out.write("\\newcommand{\\%s}{%s\\xspace}  %% %s\n" % (name,value,desc))

def sigs(val,places=4):
    """ Turn a string into a number of significant figures (default 4)"""
    count = 0
    res   = ""
    indot = False
    for ch in val:
        if ch=='.': indot=True
        if count<places:
            if ch in "0123457890":
                res += ch
                count += 1
                continue
            res += ch
            continue
        if count>=places:
            if indot: break
            if ch in "0123457890":
                res += "0"
                continue
            if ch=='.':
                break                   # in high precision area and hit '.'
            res += ch
    return res

def icomma(i):
    """ Return an integer formatted with commas """
    if i<0:   return "-" + icomma(-i)
    if i<1000:return "%d" % i
    return icomma(i/1000) + ",%03d" % (i%1000)

def commas(i,fmt):
    """ Return a number of any format formatted with commas. """
    try:
        formatted_number = fmt % i
    except TypeError:
        return str(i)
    dot = formatted_number.find('.')
    if dot==-1: return icomma(int(formatted_number))
    int_part   = int(formatted_number[0:dot])
    float_part = formatted_number[dot:]
    return icomma(int_part) + float_part

# hr is the tag that we use for linebreaks 

class row:
    def __init__(self,data):
        self.data = data
    def __len__(self):
        return len(self.data)
    def __getitem__(self,n):
        return self.data[n]

class subhead(row):
    def __init__(self,val):
        self.data = [val]

class raw(row):
    def __init__(self,data):
        self.data = data
    

class ttable:
    """ Python class that prints formatted tables. It can also output LaTeX.
    Typesetting:
       Each entry is formatted and then typset.
       Formatting is determined by the column formatting that is provided by the caller.
       Typesetting is determined by the typesetting engine (text, html, LaTeX, etc).
       Numbers are always right-justified, text is always left-justified, and headings
       are center-justified.

       set_title(title) 
       append_head([row]) to add each heading row and
       append_data([row]) to append data rows
       append_data(ttable.HR) - add a horizontal line
       set_col_alignment(col,align) - where col=0..maxcols and align=ttable.RIGHT or ttable.LEFT or ttable.CENTER
                                (center is not implemented yet)
       set_col_totals([1,2,3,4]) - compute totals of columns 1,2,3 and 4
       typeset() to typeset
    """

    HR = "<hr>"
    SUPPRESS_ZERO="suppress_zero"
    RIGHT="RIGHT"
    LEFT="LEFT"
    CENTER="CENTER"

    def __init__(self):
        self.col_headings = []          # the col_headings; a list of lists
        self.data         = []          # the raw data; a list of lists
        self.omit_row     = []          # descriptions of rows that should be omitted
        self.col_widths   = []          # a list of how wide each of the formatted columns are
        self.col_margin   = 1
        self.col_fmt_default  = "%d"
        self.col_fmt      = {}          # format for each number
        self.title        = ""
        self.num_units    = []
        self.footer       = ""
        self.header       = None # 
        self.do_begin     = True        # do \begin{table} as a header
        self.do_end       = True        # do \end{table} as a header
        self.heading_hr_count = 1       # number of <hr> to put between heading and table body
        self.options      = set()
        self.col_alignment = {}

    def nl(self):
        """ the newline for the current mode"""
        if self.mode=='text': return "\n"
        if self.mode=='latex': return "\\\\ \n"
        if self.mode=='html': return "\n"
        raise ValueError("Unknown mode '%s'" % self.mode)

    def set_mode(self,m): self.mode = m
    def set_data(self,d): self.data = d
    def set_title(self,t): self.title = t
    def set_footer(self,footer): self.footer = footer
    def set_caption(self,c): self.caption = c
    def set_option(self,o): self.options.add(o)
    def set_col_alignment(self,col,align): self.col_alignment[col] = align
    def set_col_totals(self,totals): self.col_totals = totals
    def set_col_fmt(self,col,prefix,fmt,suffix): self.col_fmt[col] = (prefix,fmt,suffix)

    def append_head(self,values):
        """ Append a row of VALUES to the table header. The VALUES should be a list of columsn."""
        assert type(values)==type([]) or type(values)==type(())
        self.col_headings.append(values)

    def append_data(self,values):
        """ Append a ROW to the table body. The ROW should be a list of each column."""
        self.data.append(row(values))

    def append_subhead(self,values):
        self.data.append(subhead(values))

    def append_raw(self,val):
        self.data.append(raw(val))

    def ncols(self):
        " Return the number of maximum number of cols in the data"
        return max([len(r) for r in self.data])


    ################################################################

    def format_cell(self,value,colNumber):
        """ Format a value that appears in a given colNumber."""
        
        import decimal
        ret = None
        if value==None:
            return ("",self.LEFT)
        if value==0 and self.SUPPRESS_ZERO in self.options:
            return ("",self.LEFT)
        if isnumber(value):
            try:
                (prefix,fmt,suffix) = self.col_fmt[colNumber]
                fmt = prefix + commas(value,fmt) + suffix
            except KeyError:
                fmt = commas(value,self.col_fmt_default)
            ret = (fmt,self.RIGHT)
        if not ret:
            ret = (str(value),self.LEFT)
        return (ret[0], self.col_alignment.get(colNumber,ret[1]))

    def col_formatted_width(self,colNum):
        " Returns the width of column number colNum "
        maxColWidth = 0
        for r in self.col_headings:
            try:
                maxColWidth = max(maxColWidth, len(self.format_cell(r[colNum],colNum)[0]))
            except IndexError:
                pass
        for r in self.data:
            try:
                maxColWidth = max(maxColWidth, len(self.format_cell(r[colNum],colNum)[0]))
            except IndexError:
                pass
        return maxColWidth

    ################################################################

    def typeset_hr(self):
        "Output a HR."
        if self.mode=='latex':
            return "\\hline\n "
        if self.mode=='text':
            total = 0
            for col in range(0,self.cols):
                total += self.col_formatted_width(col)
            total += self.col_margin * (self.cols-1)
            return "-" * total + "\n"
        if self.mode=='html':
            return ""                   # don't insert
        raise ValueError("Unknown mode '%s'" % self.mode)

    def typeset_cell(self,formattedValue,colNumber):
        "Typeset a value for a given column number."
        import math
        align = self.col_alignment.get(colNumber,self.LEFT)
        if self.mode=='html': return formattedValue
        if self.mode=='latex': return formattedValue
        if self.mode=='text': 
            try:
                fill = (self.col_formatted_widths[colNumber]-len(formattedValue))
            except IndexError:
                fill=0
            if align==self.RIGHT:
                return " "*fill+formattedValue
            if align==self.CENTER:
                return " "*math.ceil(fill/2.0)+formattedValue+" "*math.floor(fill/2.0)
            # Must be LEFT
            if colNumber != self.cols-1: # not the last column
                return formattedValue + " "*fill
            return formattedValue               #  don't indent last column
        raise ValueError("Unknown mode '%s'" % self.mode)

    def typeset_row(self,r):
        "Actually output the ROW array."
        ret = ""
        if isinstance(r,raw):
            return r[0]
        if isinstance(r,subhead):
            # Do a blank line
            if self.mode=='text':
                ret += '\n' + r[0]
            if self.mode=='latex':
                ret += '\\\\ \n' + r[0]
            if self.mode=='html':
                ret += '<tr><th colspace=%d class="subhead">%s</th></tr>' % (self.cols,r[0])
            ret += self.nl()
            return ret

        for colNumber in range(0,len(r)):
            if colNumber>0:
                if self.mode=='latex': ret += " & "
                ret += " "*self.col_margin 
            (fmt,just)      = self.format_cell(r[colNumber],colNumber)
            val             = self.typeset_cell(fmt,colNumber)

            if self.mode=='text':
                ret += val
            if self.mode=='latex':
                ret += val.replace('%','\\%')
            if self.mode=='html':
                if just==self.RIGHT: just="style='text-align:right;'"
                elif just==self.LEFT: just=""
                ret += '<%s %s>%s</%s>' % (self.html_delim,just,val,self.html_delim)

        if self.mode=='html':
            ret ="<tr>%s</tr>\n" % ret
        ret += self.nl()
        return ret
        

    ################################################################

    def calculate_col_formatted_widths(self):
        " Calculate the width of each formatted column and return the array "
        self.col_formatted_widths = []
        for i in range(0,self.cols):
            self.col_formatted_widths.append(self.col_formatted_width(i))
        return self.col_formatted_widths

    def should_omit_row(self,r):
        if r.data==self.HR: return True
        for (a,b) in self.omit_row:
            if r[a]==b: return True
        return False

    def compute_col_totals(self,col_totals):
        " Add totals for the specified cols"
        self.cols = self.ncols()
        totals = [0] * self.cols
        for r in self.data:
            if self.should_omit_row(r):
                continue
            for col in col_totals:
                print(totals[col],r[col])
                totals[col] += r[col]
        row = ["Total"]
        for col in range(1,self.cols):
            if col in col_totals:
                row.append(totals[col])
            else:
                row.append("")
        self.append_data(self.HR)
        self.append_data(row)

    ################################################################

    def typeset(self,mode='text'):
        " Returns the typset output of the entire table"

        self.cols = self.ncols() # cache
        self.mode = mode
        if self.mode not in ['text','latex','html']:
            raise ValueError("Invalid typsetting mode "+self.mode)

        ret = []

        if self.mode=='text':
            self.calculate_col_formatted_widths()
            if self.title: ret.append(self.title + ":" + "\n")

        #
        # Start of the table
        #
        if self.header:
            if self.mode=='text':
                ret.append(self.header)
                ret.append("\n")

        if self.mode=='latex' and self.do_begin:
            try:
                colspec = self.latex_colspec
            except AttributeError:
                colspec = "r"*self.cols 
            ret.append("\\begin{tabular}{%s}\n" % colspec)
        if self.mode=='html':
            ret.append("<table>\n")

        #
        # Typeset the headings
        #
        self.html_delim = 'th'
        if self.col_headings:
            for row in self.col_headings:
                ret.append(self.typeset_row(row))
            for i in range(0,self.heading_hr_count):
                ret.append(self.typeset_hr())

        #
        # typeset each row.
        # computes the width of each row if necessary
        #
        self.html_delim = 'td'
        for r in self.data:
            # See if this row demands special processing
            if r.data==self.HR:
                ret.append(self.typeset_hr())
                continue

            # See if we should omit this row
            if self.should_omit_row(r):
                continue

            ret.append(self.typeset_row(r))
            if hasattr(self,"col_totals"):
                for col in self.col_totals:
                    totals[col] += r[col]

        # Footer 
        if self.footer:
            ret.append(self.footer + "\n")

        if self.do_end:
            if self.mode=='latex': ret.append("\\end{tabular}\n")
            if self.mode=='html':  ret.append("</table>\n")
        return "".join(ret)
        
        
