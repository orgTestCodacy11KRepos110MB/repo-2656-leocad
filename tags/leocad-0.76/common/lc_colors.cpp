#include "lc_global.h"
#include "lc_colors.h"
#include "file.h"
#include "lc_array.h"
#include "str.h"
#include <float.h>

lcColor* g_ColorList;
u32 lcNumUserColors;
u32 lcNumColors;

static const char sDefaultColors[] =
{
	"0 LDraw.org Configuration File\n"
	"0 Name: LDConfig.ldr\n"
	"0 Author: LDraw.org\n"
	"0 !LDRAW_ORG Configuration UPDATE 2009-09-05\n"
	"\n"
	"0 // LDraw Core Colors\n"
	"0 !COLOUR Black                                                 CODE   0   VALUE #212121   EDGE #595959\n"
	"0 !COLOUR Blue                                                  CODE   1   VALUE #0033B2   EDGE #333333\n"
	"0 !COLOUR Green                                                 CODE   2   VALUE #008C14   EDGE #333333\n"
	"0 !COLOUR Dark_Turquoise                                        CODE   3   VALUE #00999F   EDGE #333333\n"
	"0 !COLOUR Red                                                   CODE   4   VALUE #C40026   EDGE #333333\n"
	"0 !COLOUR Dark_Pink                                             CODE   5   VALUE #DF6695   EDGE #333333\n"
	"0 !COLOUR Brown                                                 CODE   6   VALUE #5C2000   EDGE #1E1E1E\n"
	"0 !COLOUR Light_Gray                                            CODE   7   VALUE #9C9999   EDGE #333333\n"
	"0 !COLOUR Dark_Gray                                             CODE   8   VALUE #635F52   EDGE #333333\n"
	"0 !COLOUR Light_Blue                                            CODE   9   VALUE #6BABDC   EDGE #333333\n"
	"0 !COLOUR Bright_Green                                          CODE  10   VALUE #6BEE90   EDGE #333333\n"
	"0 !COLOUR Light_Turquoise                                       CODE  11   VALUE #33A6A7   EDGE #333333\n"
	"0 !COLOUR Salmon                                                CODE  12   VALUE #FF857A   EDGE #333333\n"
	"0 !COLOUR Pink                                                  CODE  13   VALUE #F9A4C6   EDGE #333333\n"
	"0 !COLOUR Yellow                                                CODE  14   VALUE #FFDC00   EDGE #333333\n"
	"0 !COLOUR White                                                 CODE  15   VALUE #FFFFFF   EDGE #333333\n"
	"0 !COLOUR Main_Color                                            CODE  16   VALUE #7F7F7F   EDGE #333333\n"
	"0 !COLOUR Light_Green                                           CODE  17   VALUE #BAFFCE   EDGE #333333\n"
	"0 !COLOUR Light_Yellow                                          CODE  18   VALUE #FDE896   EDGE #333333\n"
	"0 !COLOUR Tan                                                   CODE  19   VALUE #E8CFA1   EDGE #333333\n"
	"0 !COLOUR Light_Violet                                          CODE  20   VALUE #D7C4E6   EDGE #333333\n"
	"0 !COLOUR Purple                                                CODE  22   VALUE #81007B   EDGE #333333\n"
	"0 !COLOUR Dark_Blue_Violet                                      CODE  23   VALUE #4732B0   EDGE #1E1E1E\n"
	"0 !COLOUR Edge_Color                                            CODE  24   VALUE #7F7F7F   EDGE #333333\n"
	"0 !COLOUR Orange                                                CODE  25   VALUE #F96000   EDGE #333333\n"
	"0 !COLOUR Magenta                                               CODE  26   VALUE #D81B6D   EDGE #333333\n"
	"0 !COLOUR Lime                                                  CODE  27   VALUE #D7F000   EDGE #333333\n"
	"0 !COLOUR Dark_Tan                                              CODE  28   VALUE #C59750   EDGE #333333\n"
	"0 !COLOUR Bright_Pink                                           CODE  29   VALUE #E4ADC8   EDGE #333333\n"
	"0 !COLOUR Very_Light_Orange                                     CODE  68   VALUE #F3CF9B   EDGE #333333\n"
	"0 !COLOUR Light_Purple                                          CODE  69   VALUE #CD6298   EDGE #333333\n"
	"0 !COLOUR Reddish_Brown                                         CODE  70   VALUE #694027   EDGE #333333\n"
	"0 !COLOUR Light_Bluish_Gray                                     CODE  71   VALUE #A3A2A4   EDGE #333333\n"
	"0 !COLOUR Dark_Bluish_Gray                                      CODE  72   VALUE #635F61   EDGE #333333\n"
	"0 !COLOUR Medium_Blue                                           CODE  73   VALUE #6E99C9   EDGE #333333\n"
	"0 !COLOUR Medium_Green                                          CODE  74   VALUE #A1C48B   EDGE #333333\n"
	"0 !COLOUR Light_Pink                                            CODE  77   VALUE #FECCCC   EDGE #333333\n"
	"0 !COLOUR Light_Flesh                                           CODE  78   VALUE #FAD7C3   EDGE #333333\n"
	"0 !COLOUR Dark_Purple                                           CODE  85   VALUE #342B75   EDGE #1E1E1E\n"
	"0 !COLOUR Dark_Flesh                                            CODE  86   VALUE #7C5C45   EDGE #333333\n"
	"0 !COLOUR Blue_Violet                                           CODE  89   VALUE #6C81B7   EDGE #333333\n"
	"0 !COLOUR Flesh                                                 CODE  92   VALUE #CC8E68   EDGE #333333\n"
	"0 !COLOUR Light_Salmon                                          CODE 100   VALUE #EEC4B6   EDGE #333333\n"
	"0 !COLOUR Violet                                                CODE 110   VALUE #435493   EDGE #333333\n"
	"0 !COLOUR Medium_Violet                                         CODE 112   VALUE #6874AC   EDGE #333333\n"
	"0 !COLOUR Medium_Lime                                           CODE 115   VALUE #C7D23C   EDGE #333333\n"
	"0 !COLOUR Aqua                                                  CODE 118   VALUE #B7D7D5   EDGE #333333\n"
	"0 !COLOUR Light_Lime                                            CODE 120   VALUE #D9E4A7   EDGE #333333\n"
	"0 !COLOUR Light_Orange                                          CODE 125   VALUE #EAB891   EDGE #333333\n"
	"0 !COLOUR Very_Light_Bluish_Gray                                CODE 151   VALUE #E5E4DE   EDGE #333333\n"
	"0 !COLOUR Bright_Light_Orange                                   CODE 191   VALUE #E8AB2D   EDGE #333333\n"
	"0 !COLOUR Bright_Light_Blue                                     CODE 212   VALUE #9FC3E9   EDGE #333333\n"
	"0 !COLOUR Rust                                                  CODE 216   VALUE #8F4C2A   EDGE #333333\n"
	"0 !COLOUR Bright_Light_Yellow                                   CODE 226   VALUE #FDEA8C   EDGE #333333\n"
	"0 !COLOUR Sky_Blue                                              CODE 232   VALUE #7DBBDD   EDGE #333333\n"
	"0 !COLOUR Dark_Blue                                             CODE 272   VALUE #001D68   EDGE #1E1E1E\n"
	"0 !COLOUR Dark_Green                                            CODE 288   VALUE #27462C   EDGE #333333\n"
	"0 !COLOUR Dark_Brown                                            CODE 308   VALUE #352100   EDGE #000000\n"
	"0 !COLOUR Maersk_Blue                                           CODE 313   VALUE #35A2BD   EDGE #333333\n"
	"0 !COLOUR Dark_Red                                              CODE 320   VALUE #78001C   EDGE #333333\n"
	"0 !COLOUR Sand_Red                                              CODE 335   VALUE #BF8782   EDGE #333333\n"
	"0 !COLOUR Earth_Orange                                          CODE 366   VALUE #D18304   EDGE #333333\n"
	"0 !COLOUR Sand_Purple                                           CODE 373   VALUE #845E84   EDGE #333333\n"
	"0 !COLOUR Sand_Green                                            CODE 378   VALUE #A0BCAC   EDGE #333333\n"
	"0 !COLOUR Sand_Blue                                             CODE 379   VALUE #6A7A96   EDGE #333333\n"
	"0 !COLOUR Medium_Orange                                         CODE 462   VALUE #FE9F06   EDGE #333333\n"
	"0 !COLOUR Dark_Orange                                           CODE 484   VALUE #B33E00   EDGE #333333\n"
	"0 !COLOUR Very_Light_Gray                                       CODE 503   VALUE #E6E3DA   EDGE #333333\n"
	"\n"
	"0 // LDraw Transparent Core Colors\n"
	"0 !COLOUR Trans_Dark_Black                                      CODE  32   VALUE #000000   EDGE #000000   ALPHA 128\n"
	"0 !COLOUR Trans_Dark_Blue                                       CODE  33   VALUE #0020A0   EDGE #000064   ALPHA 128\n"
	"0 !COLOUR Trans_Green                                           CODE  34   VALUE #066432   EDGE #002800   ALPHA 128\n"
	"0 !COLOUR Trans_Red                                             CODE  36   VALUE #C40026   EDGE #880000   ALPHA 128\n"
	"0 !COLOUR Trans_Purple                                          CODE  37   VALUE #640061   EDGE #280025   ALPHA 128\n"
	"0 !COLOUR Trans_Black                                           CODE  40   VALUE #635F52   EDGE #272316   ALPHA 128\n"
	"0 !COLOUR Trans_Light_Blue                                      CODE  41   VALUE #AEEFEC   EDGE #72B3B0   ALPHA 128\n"
	"0 !COLOUR Trans_Neon_Green                                      CODE  42   VALUE #C0FF00   EDGE #84C300   ALPHA 128\n"
	"0 !COLOUR Trans_Very_Light_Blue                                 CODE  43   VALUE #C1DFF0   EDGE #85A3B4   ALPHA 128\n"
	"0 !COLOUR Trans_Dark_Pink                                       CODE  45   VALUE #DF6695   EDGE #A32A59   ALPHA 128\n"
	"0 !COLOUR Trans_Yellow                                          CODE  46   VALUE #CAB000   EDGE #8E7400   ALPHA 128\n"
	"0 !COLOUR Trans_Clear                                           CODE  47   VALUE #FFFFFF   EDGE #C3C3C3   ALPHA 128\n"
	"0 !COLOUR Trans_Neon_Orange                                     CODE  57   VALUE #F96000   EDGE #BD2400   ALPHA 128\n"
	"0 !COLOUR Trans_Neon_Blue                                       CODE 143   VALUE #CFE2F7   EDGE #93A6BB   ALPHA 128\n"
	"0 !COLOUR Trans_Neon_Yellow                                     CODE 157   VALUE #FFF67B   EDGE #C3BA3F   ALPHA 128\n"
	"0 !COLOUR Trans_Orange                                          CODE 182   VALUE #E09864   EDGE #A45C28   ALPHA 128\n"
	"0 !COLOUR Trans_Bright_Green                                    CODE 227   VALUE #D9E4A7   EDGE #9DA86B   ALPHA 128\n"
	"0 !COLOUR Trans_Medium_Blue                                     CODE 228   VALUE #55A5AF   EDGE #196973   ALPHA 128\n"
	"0 !COLOUR Trans_Pink                                            CODE 230   VALUE #E4ADC8   EDGE #A8718C   ALPHA 128\n"
	"0 !COLOUR Trans_Light_Orange                                    CODE 231   VALUE #E8AB2D   EDGE #AC6F00   ALPHA 128\n"
	"0 !COLOUR Trans_Light_Yellow                                    CODE 234   VALUE #F9D62E   EDGE #BD9A00   ALPHA 128\n"
	"0 !COLOUR Trans_Light_Purple                                    CODE 236   VALUE #96709F   EDGE #5A3463   ALPHA 128\n"
	"0 !COLOUR TLG_Transparent_Reddish_Lilac                         CODE 284   VALUE #E0D0E5   EDGE #A494A9   ALPHA 128\n"
	"\n"
	"0 // LDraw Glitter Colors\n"
	"0 !COLOUR Glitter_Trans_Dark_Pink                               CODE 114   VALUE #DF6695   EDGE #9A2A66   ALPHA 128                   MATERIAL GLITTER VALUE #923978 FRACTION 0.17 VFRACTION 0.2 SIZE 1\n"
	"0 !COLOUR Glitter_Trans_Clear                                   CODE 117   VALUE #FFFFFF   EDGE #C3C3C3   ALPHA 128                   MATERIAL GLITTER VALUE #FFFFFF FRACTION 0.08 VFRACTION 0.1 SIZE 1\n"
	"0 !COLOUR Glitter_Trans_Purple                                  CODE 129   VALUE #640061   EDGE #280025   ALPHA 128                   MATERIAL GLITTER VALUE #8C00FF FRACTION 0.3 VFRACTION 0.4 SIZE 1\n"
	"\n"
	"0 // LDraw Milky Colors\n"
	"0 !COLOUR Glow_In_Dark_Opaque                                   CODE  21   VALUE #E0FFB0   EDGE #A4C374   ALPHA 250   LUMINANCE 15\n"
	"0 !COLOUR Milky_White                                           CODE  79   VALUE #FFFFFF   EDGE #C3C3C3   ALPHA 224\n"
	"0 !COLOUR Glow_In_Dark_Trans                                    CODE 294   VALUE #BDC6AD   EDGE #818A71   ALPHA 250   LUMINANCE 15\n"
	"\n"
	"0 // LDraw Pearl Colors\n"
	"0 !COLOUR Copper                                                CODE 134   VALUE #AE7A59   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Light_Gray                                      CODE 135   VALUE #9CA3A8   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Metal_Blue                                            CODE 137   VALUE #7988A1   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Light_Gold                                      CODE 142   VALUE #DCBC81   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Dark_Gray                                       CODE 148   VALUE #575857   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Very_Light_Grey                                 CODE 150   VALUE #ABADAC   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Flat_Dark_Gold                                        CODE 178   VALUE #B48455   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Flat_Silver                                           CODE 179   VALUE #898788   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_White                                           CODE 183   VALUE #F2F3F2   EDGE #333333                               PEARLESCENT\n"
	"0 !COLOUR Pearl_Gold                                            CODE 297   VALUE #AA7F2E   EDGE #333333                               PEARLESCENT\n"
	"\n"
	"0 // LDraw Chrome Colors\n"
	"0 !COLOUR Chrome_Antique_Brass                                  CODE  60   VALUE #645A4C   EDGE #281E10                               CHROME\n"
	"0 !COLOUR Chrome_Blue                                           CODE  61   VALUE #5C66A4   EDGE #202A68                               CHROME\n"
	"0 !COLOUR Chrome_Green                                          CODE  62   VALUE #3CB371   EDGE #007735                               CHROME\n"
	"0 !COLOUR Chrome_Pink                                           CODE  63   VALUE #AA4D8E   EDGE #6E1152                               CHROME\n"
	"0 !COLOUR Chrome_Black                                          CODE  64   VALUE #1B2A34   EDGE #000000                               CHROME\n"
	"0 !COLOUR Chrome_Gold                                           CODE 334   VALUE #E16E13   EDGE #A53200                               CHROME\n"
	"0 !COLOUR Chrome_Silver                                         CODE 383   VALUE #E0E0E0   EDGE #A4A4A4                               CHROME\n"
	"0 !COLOUR Electric_Contact_Alloy                                CODE 494   VALUE #D0D0D0   EDGE #6E6E6E                               METAL\n"
	"0 !COLOUR Electric_Contact_Copper                               CODE 495   VALUE #AE7A59   EDGE #723E1D                               METAL\n"
	"\n"
	"0 // LDraw Rubber Colors\n"
	"0 !COLOUR Rubber_Yellow                                         CODE  65   VALUE #F5CD2F   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Trans_Yellow                                   CODE  66   VALUE #CAB000   EDGE #8E7400   ALPHA 128                   RUBBER\n"
	"0 !COLOUR Rubber_Trans_Clear                                    CODE  67   VALUE #FFFFFF   EDGE #C3C3C3   ALPHA 128                   RUBBER\n"
	"0 !COLOUR Rubber_Black                                          CODE 256   VALUE #212121   EDGE #595959                               RUBBER\n"
	"0 !COLOUR Rubber_Blue                                           CODE 273   VALUE #0033B2   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Red                                            CODE 324   VALUE #C40026   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_Light_Gray                                     CODE 375   VALUE #C1C2C1   EDGE #333333                               RUBBER\n"
	"0 !COLOUR Rubber_White                                          CODE 511   VALUE #FFFFFF   EDGE #333333                               RUBBER\n"
	"\n"
	"0 // LDraw Speckle Colors\n"
	"0 !COLOUR Speckle_Black_Copper                                  CODE  75   VALUE #000000   EDGE #595959                               MATERIAL SPECKLE VALUE #AE7A59 FRACTION 0.4 MINSIZE 1 MAXSIZE 3\n"
	"0 !COLOUR Speckle_Dark_Bluish_Gray_Silver                       CODE  76   VALUE #635F61   EDGE #595959                               MATERIAL SPECKLE VALUE #595959 FRACTION 0.4 MINSIZE 1 MAXSIZE 3\n"
	"0 !COLOUR Speckle_Black_Silver                                  CODE 132   VALUE #000000   EDGE #595959                               MATERIAL SPECKLE VALUE #595959 FRACTION 0.4 MINSIZE 1 MAXSIZE 3\n"
};

void lcColorConfig::Load(lcFile& File)
{
	mColors.RemoveAll();
	mColorGroups.RemoveAll();

	char Buf[1024];

	while (File.ReadLine(Buf, sizeof(Buf)))
	{
		char* ptr = Buf;
		String Token;

		Token = GetToken(ptr);
		if (Token.CompareNoCase("0"))
			continue;

		Token = GetToken(ptr);
		if (Token.CompareNoCase("!COLOUR"))
			continue;

		lcColor Color;

		Color.Code = -1;
		Color.Translucent = false;
		Color.Value[0] = FLT_MAX;
		Color.Value[1] = FLT_MAX;
		Color.Value[2] = FLT_MAX;
		Color.Value[3] = 1.0f;

		Color.Name = GetToken(ptr);

		for (Token = GetToken(ptr); !Token.IsEmpty(); Token = GetToken(ptr))
		{
			if (!Token.CompareNoCase("CODE"))
			{
				Token = GetToken(ptr);
				Color.Code = atoi(Token);
			}
			else if (!Token.CompareNoCase("VALUE"))
			{
				Token = GetToken(ptr);
				if (Token[0] == '#')
					Token[0] = ' ';

				int Value;
				sscanf(Token, "%x", &Value);

				Color.Value[2] = (float)(Value & 0xff) / 255.0f;
				Value >>= 8;
				Color.Value[1] = (float)(Value & 0xff) / 255.0f;
				Value >>= 8;
				Color.Value[0] = (float)(Value & 0xff) / 255.0f;
			}
			else if (!Token.CompareNoCase("EDGE"))
			{
				Token = GetToken(ptr); // Ignored.
			}
			else if (!Token.CompareNoCase("ALPHA"))
			{
				Token = GetToken(ptr);
				int Value = atoi(Token);
				Color.Value[3] = (float)(Value & 0xff) / 255.0f;
				if (Value != 255)
					Color.Translucent = true;
			}
			else if (!Token.CompareNoCase("CHROME") || !Token.CompareNoCase("PEARLESCENT") || !Token.CompareNoCase("RUBBER") ||
			         !Token.CompareNoCase("MATTE_METALIC") || !Token.CompareNoCase("METAL"))
			{
				// Ignored.
			}
			else if (!Token.CompareNoCase("MATERIAL"))
			{
				break; // Material is always last so ignore it and the rest of the line.
			}
		}

		// Check if the new color is valid.
		if (Color.Code == -1 || Color.Value[0] == FLT_MAX)
			continue;

		// Check for duplicates.
		for (int i = 0; i < mColors.GetSize(); i++)
		{
			if (mColors[i].Code == Color.Code)
			{
				mColors.RemoveIndex(i);
				break;
			}
		}

		// Replace underscores with spaces.
		for (char* Ptr = strchr((char*)Color.Name, '_'); Ptr; Ptr = strchr(Ptr, '_'))
			*Ptr = ' ';

		mColors.Add(Color);
	}

	lcColor DefaultColors[] =
	{
		{  16, { 0.5000f, 0.5000f, 0.5000f, 1.0000f }, false, "Default" },
		{  24, { 0.2000f, 0.2000f, 0.2000f, 1.0000f }, false, "Edge" },
		{  -1, { 0.8980f, 0.2980f, 0.4000f, 1.0000f }, false, "Selected" },
		{  -2, { 0.4000f, 0.2980f, 0.8980f, 1.0000f }, false, "Focused" },
	};

	// Reorder colors.
	lcColor Default;
	Default.Code = -1;
	lcColor Edge;
	Edge.Code = -1;

	for (int i = 0; i < mColors.GetSize(); i++)
	{
		if (mColors[i].Code != 16)
			continue;

		Default = mColors[i];
		mColors.RemoveIndex(i);
		break;
	}

	for (int i = 0; i < mColors.GetSize(); i++)
	{
		if (mColors[i].Code != 24)
			continue;

		Edge = mColors[i];
		mColors.RemoveIndex(i);
		break;
	}

	if (Default.Code == -1)
		Default = DefaultColors[0];
	mColors.Add(Default);

	if (Edge.Code == -1)
		Edge = DefaultColors[1];
	mColors.Add(Edge);

	mColors.Add(DefaultColors[2]);
	mColors.Add(DefaultColors[3]);
}

void lcColorConfig::LoadDefault()
{
	lcFileMem File;
	File.Write(sDefaultColors, sizeof(sDefaultColors));
	File.Seek(0, SEEK_SET);
	Load(File);
}



/*
static lcColor sDefaultColors[] =
{
	{   0, { 0.1294f, 0.1294f, 0.1294f, 1.0000f }, false, "Black" },
	{   1, { 0.0000f, 0.2000f, 0.6980f, 1.0000f }, false, "Blue" },
	{   2, { 0.0000f, 0.5490f, 0.0784f, 1.0000f }, false, "Green" },
	{   3, { 0.0000f, 0.6000f, 0.6235f, 1.0000f }, false, "Teal" },
	{   4, { 0.7686f, 0.0000f, 0.1490f, 1.0000f }, false, "Red" },
	{   5, { 0.8745f, 0.4000f, 0.5843f, 1.0000f }, false, "Dark Pink" },
	{   6, { 0.3608f, 0.1255f, 0.0000f, 1.0000f }, false, "Brown" },
	{   7, { 0.7569f, 0.7608f, 0.7569f, 1.0000f }, false, "Gray" },
	{   8, { 0.3882f, 0.3725f, 0.3216f, 1.0000f }, false, "Dark Gray" },
	{   9, { 0.4196f, 0.6706f, 0.8627f, 1.0000f }, false, "Light Blue" },
	{  10, { 0.4196f, 0.9333f, 0.5647f, 1.0000f }, false, "Bright Green" },
	{  11, { 0.2000f, 0.6510f, 0.6549f, 1.0000f }, false, "Turquiose" },
	{  12, { 1.0000f, 0.5216f, 0.4784f, 1.0000f }, false, "Light Red" },
	{  13, { 0.9765f, 0.6431f, 0.7765f, 1.0000f }, false, "Pink" },
	{  14, { 1.0000f, 0.8627f, 0.0000f, 1.0000f }, false, "Yellow" },
	{  15, { 1.0000f, 1.0000f, 1.0000f, 1.0000f }, false, "White" },
	{  17, { 0.7294f, 1.0000f, 0.8078f, 1.0000f }, false, "Light Green" },
	{  18, { 0.9922f, 0.9098f, 0.5882f, 1.0000f }, false, "Light Yellow" },
	{  19, { 0.9098f, 0.8118f, 0.6314f, 1.0000f }, false, "Tan" },
	{  20, { 0.8431f, 0.7686f, 0.9020f, 1.0000f }, false, "Light Violet" },
	{  21, { 0.8784f, 1.0000f, 0.6902f, 0.9804f }, false, "Phosphor White" },
	{  22, { 0.5059f, 0.0000f, 0.4824f, 1.0000f }, false, "Violet" },
	{  23, { 0.2784f, 0.1961f, 0.6902f, 1.0000f }, false, "Violet Blue" },
	{  25, { 0.9765f, 0.3765f, 0.0000f, 1.0000f }, false, "Orange" },
	{  26, { 0.8471f, 0.1059f, 0.4275f, 1.0000f }, false, "Magenta" },
	{  27, { 0.8431f, 0.9412f, 0.0000f, 1.0000f }, false, "Lime" },
	{  28, { 0.7725f, 0.5922f, 0.3137f, 1.0000f }, false, "Dark Tan" },
	{  33, { 0.0000f, 0.1255f, 0.6275f, 0.5020f },  true, "Trans Blue" },
	{  34, { 0.0235f, 0.3921f, 0.1961f, 0.5020f },  true, "Trans Green" },
	{  36, { 0.7686f, 0.0000f, 0.1490f, 0.5020f },  true, "Trans Red" },
	{  37, { 0.3922f, 0.0000f, 0.3804f, 0.5020f },  true, "Trans Violet" },
	{  40, { 0.3882f, 0.3725f, 0.3216f, 0.5020f },  true, "Trans Gray" },
	{  41, { 0.6824f, 0.9373f, 0.9255f, 0.5020f },  true, "Trans Light Cyan" },
	{  42, { 0.7529f, 1.0000f, 0.0000f, 0.5020f },  true, "Trans Flu Lime" },
	{  45, { 0.8745f, 0.4000f, 0.5843f, 0.5020f },  true, "Trans Pink" },
	{  46, { 0.7922f, 0.6902f, 0.0000f, 0.5020f },  true, "Trans Yellow" },
	{  47, { 1.0000f, 1.0000f, 1.0000f, 0.5020f },  true, "Clear" },
	{  57, { 0.9765f, 0.3765f, 0.0000f, 0.5020f },  true, "Trans Flu Orange" },
	{  70, { 0.4118f, 0.2510f, 0.1529f, 1.0000f }, false, "Reddish Brown" },
	{  71, { 0.6392f, 0.6353f, 0.6431f, 1.0000f }, false, "Stone Gray" },
	{  72, { 0.3882f, 0.3725f, 0.3804f, 1.0000f }, false, "Dark Stone Gray" },
	{ 134, { 0.5765f, 0.5294f, 0.4039f, 1.0000f }, false, "Pearl Copper" },
	{ 135, { 0.6706f, 0.6784f, 0.6745f, 1.0000f }, false, "Pearl Gray" },
	{ 137, { 0.4157f, 0.4784f, 0.5882f, 1.0000f }, false, "Pearl Sand Blue" },
	{ 142, { 0.8431f, 0.6627f, 0.2941f, 1.0000f }, false, "Pearl Gold" },
	{ 256, { 0.1294f, 0.1294f, 0.1294f, 1.0000f }, false, "Rubber Black" },
	{ 272, { 0.0000f, 0.1137f, 0.4078f, 1.0000f }, false, "Dark Blue" },
	{ 273, { 0.0000f, 0.2000f, 0.6980f, 1.0000f }, false, "Rubber Blue" },
	{ 288, { 0.1529f, 0.2745f, 0.1725f, 1.0000f }, false, "Dark Green" },
	{ 320, { 0.4706f, 0.0000f, 0.1098f, 1.0000f }, false, "Dark Red" },
	{ 324, { 0.7686f, 0.0000f, 0.1490f, 1.0000f }, false, "Rubber Red" },
	{ 334, { 0.8824f, 0.4314f, 0.0745f, 1.0000f }, false, "Chrome Gold" },
	{ 335, { 0.7490f, 0.5294f, 0.5098f, 1.0000f }, false, "Sand Red" },
	{ 366, { 0.8196f, 0.5137f, 0.0157f, 1.0000f }, false, "Earth Orange" },
	{ 373, { 0.5176f, 0.3686f, 0.5176f, 1.0000f }, false, "Sand Violet" },
	{ 375, { 0.7569f, 0.7608f, 0.7569f, 1.0000f }, false, "Rubber Gray" },
	{ 378, { 0.6275f, 0.7373f, 0.6745f, 1.0000f }, false, "Sand Green" },
	{ 379, { 0.4157f, 0.4784f, 0.5882f, 1.0000f }, false, "Sand Blue" },
	{ 383, { 0.8784f, 0.8784f, 0.8784f, 1.0000f }, false, "Chrome Silver" },
	{ 462, { 0.9961f, 0.6235f, 0.0235f, 1.0000f }, false, "Light Orange" },
	{ 484, { 0.7020f, 0.2431f, 0.0000f, 1.0000f }, false, "Dark Orange" },
	{ 494, { 0.8157f, 0.8157f, 0.8157f, 1.0000f }, false, "Electric Contact" },
	{ 503, { 0.9020f, 0.8902f, 0.8549f, 1.0000f }, false, "Light Gray" },
	{ 511, { 1.0000f, 1.0000f, 1.0000f, 1.0000f }, false, "Rubber White" },

	{  29, { 0.8941f, 0.6784f, 0.7843f, 1.0000f }, false, "Light Purple" },
	{  69, { 0.8039f, 0.3843f, 0.5961f, 1.0000f }, false, "Bright Purple" },
	{  73, { 0.4314f, 0.6000f, 0.7882f, 1.0000f }, false, "Medium Blue" },
	{  74, { 0.6314f, 0.7686f, 0.5451f, 1.0000f }, false, "Medium Green" },
	{  77, { 0.9961f, 0.8000f, 0.8000f, 1.0000f }, false, "Paradisa Pink" },
	{  78, { 0.9804f, 0.8431f, 0.7647f, 1.0000f }, false, "Light Flesh" },
	{  79, { 1.0000f, 1.0000f, 1.0000f, 0.8784f },  true, "Translucent White" },
	{  85, { 0.2039f, 0.1686f, 0.4588f, 1.0000f }, false, "Medium Lilac" },
	{  86, { 0.4863f, 0.3608f, 0.2706f, 1.0000f }, false, "Dark Flesh" },
	{  89, { 0.6078f, 0.6980f, 0.9373f, 1.0000f }, false, "Royal Blue" },
	{  92, { 0.8000f, 0.5569f, 0.4078f, 1.0000f }, false, "Flesh" },
	{ 151, { 0.8980f, 0.8941f, 0.8706f, 1.0000f }, false, "Light Stone" },
	{ 313, { 0.2078f, 0.6353f, 0.7412f, 1.0000f }, false, "Maersk Blue" },

	{  16, { 0.5000f, 0.5000f, 0.5000f, 1.0000f }, false, "Default" },
	{  24, { 0.2000f, 0.2000f, 0.2000f, 1.0000f }, false, "Edge" },
	{  -1, { 0.8980f, 0.2980f, 0.4000f, 1.0000f }, false, "Selected" },
	{  -2, { 0.4000f, 0.2980f, 0.8980f, 1.0000f }, false, "Focused" },
};
*/
