#include "base.h"


#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>

#ifdef OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <stdarg.h>
	#include <windows.h>
	#include <winbase.h>
	#include <winnt.h>
#endif
#ifdef OS_LINUX
	#include <unistd.h>
	#include <dirent.h>
	#include <stdarg.h>
	#include <sys/timeb.h>
	#include <sys/stat.h>
#endif



string::string()
{
//	printf("init\n");
	init(sizeof(char));
}

string::string(const char *str)
{
//	printf("init char*\n");
	init(sizeof(char));
	int l = strlen(str);
	resize(l);
	if (l > 0)
		memcpy(data, str, l);
//	printf("--   %s\n", c_str());
}

string::string(const char *str, int l)
{
//	printf("init len\n");
	init(sizeof(char));
	resize(l);
	if (l > 0)
		memcpy(data, str, l);
}

string::string(const string &s)
{
//	printf("init string\n");
	init(sizeof(char));
	assign(&s);
}

void string::__init__()
{
	init(sizeof(char));
}

string::~string()
{
//	printf("~     %d", num);
//	printf("'%s'\n", c_str());
	//printf("   %c\n", ((char*)data)[0]);
	clear();
//	printf("/~\n");
}

string string::substr(int start, int length) const
{
	string r;
	if (start >= num)
		return r;
	if (start < 0){
		// start from the end
		start = num + start;
		if (start < 0)
			return r;
	}
	if (length < 0){
		length = num - start + length + 1;
	}
	if (start + length > num)
		length = num - start;
	if (length > 0){
		r.resize(length);
		memcpy(r.data, &((char*)data)[start], length);
	}
	return r;
}

string string::head(int size) const
{	return substr(0, size);	}

string string::tail(int size) const
{	return substr(num - size, size);	}

int string::find(const string &s, int start) const
{
	char *b = (char*)data;
	char *aa = (char*)s.data;
	for (int i=start;i<num - s.num + 1;i++){
		bool ok = true;
		for (int j=0;j<s.num;j++)
			if (b[i + j] != aa[j]){
				ok = false;
				break;
			}
		if (ok)
			return i;
	}
	return -1;
}

int string::rfind(const string &s, int start) const
{
	char *b = (char*)data;
	char *aa = (char*)s.data;
	if (start < 0)
		start = num - 1;
	for (int i=start;i>=0;i--){
		bool ok = true;
		for (int j=0;j<s.num;j++)
			if (b[i + j] != aa[j]){
				ok = false;
				break;
			}
		if (ok)
			return i;
	}
	return -1;
}

int string::compare(const string &s) const
{
	char *a = (char*)data;
	int n = num;
	if (num > s.num)
		n = s.num;
	for (int i=0;i<n;i++){
		if (s[i] != a[i])
			return (int)(unsigned char)a[i] - (int)(unsigned char)s[i];
	}
	return num - s.num;
}

inline int ichar(char a)
{
	if ((a >= 'A') && (a <= 'Z'))
		return (int)(unsigned char)a - (int)(unsigned char)'A' + (int)(unsigned char)'a';
	return (int)(unsigned char)a;
}

int string::icompare(const string &s) const
{
	char *a = (char*)data;
	int n = num;
	if (num > s.num)
		n = s.num;
	for (int i=0;i<n;i++){
		if (ichar(s[i]) != ichar(a[i]))
			return ichar(a[i]) - ichar(s[i]);
	}
	return num - s.num;
}

string string::_reverse() const
{
	string r;
	r.resize(num);
	char *a = (char*)data;
	char *b = (char*)r.data;
	for (int i=0;i<num;i++)
		b[num - i - 1] = a[i];
	return r;
}

Array<string> string::explode(const string &s) const
{
	Array<string> r;

	int pos = 0;
	while(true){
		int pos2 = find(s, pos);
		if (pos2 < 0)
			break;

		r.add(substr(pos, pos2 - pos));

		pos = s.num + pos2;
	}
	if ((r.num > 0) || (pos < num))
		r.add(substr(pos, num - pos));
	return r;
}


void string::replace0(int start, int length, const string &str)
{
	if (start + length > num)
		return;
	char *s = (char*)data;
	int d = str.num - length;
	if (d > 0){
		resize(num + d);
		s = (char*)data;
		for (int i=num-1;i>=start+length;i--)
			s[i] = s[i - d];
	}
	memcpy(&s[start], str.data, str.num);
	if (d < 0){
		for (int i=start+str.num;i<num+d;i++)
			s[i] = s[i - d];
		resize(num + d);
	}
}

string string::_replace(const string &sub, const string &by) const
{
	string r = *this;
	int i = r.find(sub, 0);
	while (i >= 0){
		r.replace0(i, sub.num, by);
		i = r.find(sub, i + by.num);
	}
	return r;
}

string string::lower() const
{
	string r = *this;
	for (int i=0;i<r.num;i++)
		if ((r[i] >= 'A') && (r[i] <= 'Z'))
			r[i] += 'a' - 'A';
	return r;
}

string string::upper() const
{
	string r = *this;
	for (int i=0;i<r.num;i++)
		if ((r[i] >= 'a') && (r[i] <= 'z'))
			r[i] += 'A' - 'a';
	return r;
}

#define STR_SMALL_STACK_DEPTH			32
#define STR_LARGE_STACK_DEPTH			8
#define STR_SMALL_SIZE					256

static int _current_stack_small_pos_ = 0;
static char _stack_small_str_[STR_SMALL_STACK_DEPTH][STR_SMALL_SIZE];
static int _current_stack_large_pos_ = 0;
static char *_stack_large_str_[STR_LARGE_STACK_DEPTH];
inline char *get_str(int size)
{
	if (size < STR_SMALL_SIZE){
		_current_stack_small_pos_ = (_current_stack_small_pos_ + 1) % STR_SMALL_STACK_DEPTH;
		return _stack_small_str_[_current_stack_small_pos_];
	}else{
		_current_stack_large_pos_ = (_current_stack_large_pos_ + 1) % STR_LARGE_STACK_DEPTH;
		if (_stack_large_str_[_current_stack_large_pos_])
			delete[]_stack_large_str_[_current_stack_large_pos_];
		_stack_large_str_[_current_stack_large_pos_] = new char[size + 1];
		return _stack_large_str_[_current_stack_large_pos_];
	}
}


const char *string::c_str() const
{
	//reserve(num + 1);
	if (allocated > num){
		((char*)data)[num] = 0;
		return (const char*)data;
	}
	char *s = get_str(num);
	if (num > 0)
		memcpy(s, data, num);
	s[num] = 0;
	return s;
}


// transposes path-strings to the current operating system
// accepts windows and linux paths ("/" and "\\")
string string::sys_filename() const
{
	string r = *this;
#ifdef OS_WINDOWS
	for (int i=0;i<r.num;i++)
		if (r[i]=='/')
			r[i]='\\';
#endif
#ifdef OS_LINUX
	for (int i=0;i<r.num;i++)
		if (r[i]=='\\')
			r[i]='/';
#endif
	return r;
}

// ends with '/' or '\'
string string::dirname() const
{
	int i = max(rfind("/"), rfind("\\"));
	if (i >= 0)
		return head(i + 1);
	return "";
}

string string::basename() const
{
	int i = max(rfind("/"), rfind("\\"));
	if (i >= 0)
		return tail(num - i - 1);
	return *this;
}

// make sure the name ends with a shlash
void string::dir_ensure_ending()
{
	if (num > 0){
		char lc = (*this)[num - 1];
		if ((lc != '/') && (lc != '\\'))
			add('/');
	}
}

// remove "/../"
string string::no_recursion() const
{
	string str = *this;
	int l1,l2,l3;
	for (l1=str.num-2;l1>=0;l1--)
		if ((str[l1]=='.')&&(str[l1+1]=='.')){
			for (l2=l1-2;l2>=0;l2--)
				if ((str[l2]=='/')||(str[l2]=='\\')){
					int ss=str.num+l2-l1-2;
					for (l3=l2;l3<ss;l3++)
						str[l3]=str[l3+l1-l2+2];
					str.resize(ss);
					l1=l2;
					break;
				}
		}
	return str;
}

string string::extension() const
{
	int pos = rfind(".");
	if (pos >= 0)
		return tail(num - pos - 1).lower();
	return "";
}

static bool format_locale_set = false;

// connecting strings
string format(const string str,...)
{
	string tmp;
	va_list args;

    // retrieve the variable arguments
    va_start(args, str);

	//if (!format_locale_set){
		setlocale(LC_NUMERIC, "C");
		//format_locale_set = true;
	//}

#ifdef OS_WINDOWS
	int len = _vscprintf(str.c_str(), args);
#else
	int len = vsnprintf(NULL, 0, str.c_str(), args);
#endif
	tmp.resize(len + 1);
    va_start(args, str);
    vsprintf((char*)tmp.data, str.c_str(), args); // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
	tmp.resize(len);
	va_end(args);
	
	return tmp;
#if 0
	char *tmp=_file_get_str_();
	tmp[0]=0;

	va_list marker;
	va_start(marker,str);

	int l=0,s=strlen(str);
	for (int i=0;i<s;i++){
		if ((str[i]=='%')&&(str[i+1]=='s')){
			strcat(tmp,va_arg(marker,char*));
			i++;
			l=strlen(tmp);
		}else if ((str[i]=='%')&&(str[i+1]=='d')){
			strcat(tmp,i2s(va_arg(marker,int)));
			i++;
			l=strlen(tmp);
		}else if ((str[i]=='%')&&(str[i+1]=='f')){
			int fl=3;
			if (str[i+2]==':'){
				fl=str[i+3]-'0';
				i+=3;
			}else
				i++;
			strcat(tmp,f2s((float)va_arg(marker,double),fl));
			l=strlen(tmp);
		}else if ((str[i]=='%')&&(str[i+1]=='v')){
			int fl=3;
			if (str[i+2]==':'){
				fl=str[i+3]-'0';
				i+=3;
			}else
				i++;
			/*float *v=(float*)&va_arg(marker,double);
			va_arg(marker,float);
			va_arg(marker,float);
			strcat(tmp,"( ");
			strcat(tmp,f2s(v[0],fl));
			strcat(tmp," , ");
			strcat(tmp,f2s(v[1],fl));
			strcat(tmp," , ");
			strcat(tmp,f2s(v[2],fl));
			strcat(tmp," )");
			l=strlen(tmp);*/
msg_write>Error("Todo:  %v");
		}else{
			tmp[l]=str[i];
			tmp[l+1]=0;
			l++;
		}
	}
	va_end(marker);

	return tmp;
#endif
}

// cut the string at the position of a substring
/*void strcut(char *str,const char *dstr)
{
	if (strstr(str,dstr))
		strstr(str,dstr)[0]=0;
}*/

// convert an integer to a string (with a given number of decimals)
string i2s2(int i,int l)
{
	string r;
	for (int n=l-1;n>=0;n--){
		r.add(i%10+48);
		i /= 10;
	}
	return r;
}

// convert an integer to a string
string i2s(int i)
{
	string r;	
	int l=0;
	bool m=false;
	if (i<0){
		i=-i;
		m=true;
	}
	char a[128];
	while (1){
		a[l]=(i%10)+48;
		l++;
		i=(int)(i/10);
		if (i==0)
			break;
	}
	if (m){
		a[l]='-';
		l++;
	}
	r.resize(l);
	for (int j=0;j<l;j++)
		r[l-j-1]=a[j];
	return r;
}

// convert a float to a string
string f2s(float f,int dez)
{
	/*strcpy(str,"");
	if (f<0){
		strcat(str,"-");
		f=-f;
	}
	strcat(str,i2s(int(f)));
	if (dez>0){
		strcat(str,",");
		int e=1;
		for (int i=0;i<dez;i++)
			e*=10;
		strcat(str,i2sl(int(f*(float)e)%e,dez));
	}*/
	if (dez > 9)
		dez = 9;
	char tmp[128], fmt[8];
	strcpy(fmt, "%.0f");
	fmt[2] += dez;
	sprintf(tmp, fmt, f);
	string t = string(tmp);
	for (int i=0;i<t.num;i++)
		if (t[i] == ',')
			t[i] = '.';
	return t;
}

// convert a float to a string
string f2sf(float f)
{
	char tmp[128];
	sprintf(tmp, "%f", f);
	string t = string(tmp);
	for (int i=0;i<t.num;i++)
		if (t[i] == ',')
			t[i] = '.';
	return t;
}

// convert a bool to a string
string b2s(bool b)
{
	if (b)
		return string("true");
	return string("false");
}

// convert a pointer to a string
string p2s(void *p)
{
	char tmp[64];
	sprintf(tmp, "%p", p);
	return string(tmp);
}

// convert binary data to a hex-code-string
// inverted:
//    false:   12.34.56.78
//    true:    0x78.56.34.12
string string::hex(bool inverted) const
{
	string str;
	if (inverted)
		str = "0x";
	unsigned char *c_data = (unsigned char *)data;
	for (int i=0;i<num;i++){
		int dd;
		if (inverted)
			dd = c_data[num - i - 1];
		else
			dd = c_data[i];
		int c1 = (dd & 15);
		int c2 = (dd >> 4);
		if (c2 < 10)
			str.add('0' + c2);
		else
			str.add('a' + c2 - 10);
		if (c1 < 10)
			str.add('0' + c1);
		else
			str.add('a' + c1 - 10);
		if ((!inverted)&&(i < num - 1))
			str.add('.');
	}
	return str;
}

string string::unhex() const
{
	return *this;
}

string d2h(const void *data, int bytes, bool inverted)
{	return string((const char*)data, bytes).hex(inverted);	}

string ia2s(const Array<int> &a)
{
	string s = "[";
	for (int i=0;i<a.num;i++){
		if (i > 0)
			s += ", ";
		s += i2s(a[i]);
	}
	s += "]";
	return s;
}

string fa2s(const Array<float> &a)
{
	string s = "[";
	for (int i=0;i<a.num;i++){
		if (i > 0)
			s += ", ";
		s += f2s(a[i], 6);
	}
	s += "]";
	return s;
}
string ba2s(const Array<bool> &a)
{
	string s = "[";
	for (int i=0;i<a.num;i++){
		if (i > 0)
			s += ", ";
		s += b2s(a[i]);
	}
	s += "]";
	return s;
}

string sa2s(const Array<string> &a)
{
	string s = "[";
	for (int i=0;i<a.num;i++){
		if (i > 0)
			s += ", ";
		s += "\"" + a[i] + "\"";
	}
	s += "]";
	return s;
}

// convert a string to an integer
int string::_int() const
{
	bool minus=false;
	int res=0;
	for (int i=0;i<num;i++){
		if ((*this)[i]=='-')
			minus=true;
		else res=res*10+((*this)[i]-48);
	}
	if (minus)
		res=-res;
	return res;
}

int s2i(const string &s)
{	return s._int();	}

// convert a string to a float
float string::_float() const
{
	bool minus=false;
	int e=-1;
	float res=0;
	for (int i=0;i<num;i++){
		if (e>0)
			e*=10;
		if ((*this)[i]=='-'){
			minus=true;
		}else{
			if (((*this)[i]==',')||((*this)[i]=='.')){
				e=1;
			}else{
				if((*this)[i]!='\n'){
					if (e<0)
						res=res*10+((*this)[i]-48);
					else
						res+=float((*this)[i]-48)/(float)e;
				}
			}
		}
	}
	if (minus)
		res=-res;
	return res;
}


float s2f(const string &s)
{	return s._float();	}

int string::hash() const
{
	int id = 0;
	int n = num / 4;
	int *str_i = (int*)data;
	for (int i=0;i<n;i++){
		int t = str_i[i];
		id = id ^ t;
	}
	int r = num - (n * 4);
	int t = 0;
	if (r == 1)
		t = (str_i[n] & 0x000000ff);
	else if (r == 2)
		t = (str_i[n] & 0x0000ffff);
	else if (r == 3)
		t = (str_i[n] & 0x00ffffff);
	id = id ^ t;
	return id;
}

string string::trim() const
{
	return *this; // TODO
}

string implode(const Array<string> &a, const string &glue)
{
	string r;
	for (int i=0;i<a.num;i++){
		if (i > 0)
			r += glue;
		r += a[i];
	}
	return r;
}

/*
char *regex_out_match[REGEX_MAX_MATCHES];
int regex_out_pos[REGEX_MAX_MATCHES],regex_out_length[REGEX_MAX_MATCHES];

int regex_match(char *rex,char *str,int max_matches)
{
	int ss=strlen(str);
	int rs=strlen(rex);

	if ((max_matches<=0)||(max_matches>REGEX_MAX_MATCHES))
		max_matches=REGEX_MAX_MATCHES;

	int n_matches=0;

	for (int i=0;i<ss;i++){
		bool match=true;
		for (int j=0;j<rs;j++){
			if (i+j>=ss)
				match=false;
			else if (str[i+j]!=rex[j])
				match=false;
		}
		if (match){
			regex_out_pos[n_matches]=i;
			regex_out_length[n_matches]=rs;
			regex_out_match[n_matches]=&str[i];
			n_matches++;
			if (n_matches>=max_matches)
				return n_matches;
		}
	}
	return n_matches;
}*/

/*char *regex_replace(char *rex,char *str,int max_matches)
{
}*/
