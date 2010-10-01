--  Generated from glut.h
--  Date: Sun Apr  6 14:32:02 1997
--
--  Command line definitions:
--      -D__ANSI_C__ -D_LANGUAGE_C -DGENERATING_ADA_BINDING -D__unix -D__sgi
--      -D__mips -D__host_mips -D__EXTENSIONS__ -D__EDG -D__DSO__ -D__STDC__
--      -D_SYSTYPE_SVR4 -D_MODERN_C -D_MIPS_SZPTR=32 -D_MIPS_SZLONG=32
--      -D_MIPS_SZINT=32 -D_MIPS_SIM=_MIPS_SIM_ABI32
--      -D_MIPS_ISA=_MIPS_ISA_MIPS1 -D_MIPS_FPSET=16 -D_MIPSEB
--

package body Glut is

   function glutCreateWindow (title : String) return Integer is
      Result : Integer;
      c_title : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (title);
   begin
      Result := glutCreateWindow (c_title);
      Interfaces.C.Strings.Free  (c_title);
      return Result;
   end glutCreateWindow;

   procedure glutInitDisplayString (name : String) is
      c_name : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (name);
   begin
      glutInitDisplayString  (c_name);
      Interfaces.C.Strings.Free  (c_name);
   end glutInitDisplayString;

   procedure glutSetWindowTitle (title : String) is
      c_title : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (title);
   begin
      glutSetWindowTitle  (c_title);
      Interfaces.C.Strings.Free  (c_title);
   end glutSetWindowTitle;

   procedure glutSetIconTitle (title : String) is
      c_title : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (title);
   begin
      glutSetIconTitle  (c_title);
      Interfaces.C.Strings.Free  (c_title);
   end glutSetIconTitle;

   function glutBitmapLength (font : access Interfaces.C.Extensions.Void_Ptr;
     str : String) return Integer is
      Result : Integer;
      c_str : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (str);
   begin
      Result := glutBitmapLength  (font, c_str);
      Interfaces.C.Strings.Free  (c_str);
      return Result;
   end glutBitmapLength;

   function glutStrokeLength (font : access Interfaces.C.Extensions.Void_Ptr;
     str : String) return Integer is
      Result : Integer;
      c_str : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (str);
   begin
      Result := glutStrokeLength  (font, c_str);
      Interfaces.C.Strings.Free  (c_str);
      return Result;
   end glutStrokeLength;

   procedure glutAddMenuEntry (label : String; value : Integer) is
      c_label : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (label);
   begin
      glutAddMenuEntry  (c_label, value);
      Interfaces.C.Strings.Free  (c_label);
   end glutAddMenuEntry;

   procedure glutAddSubMenu (label : String; submenu : Integer) is
      c_label : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (label);
   begin
      glutAddSubMenu  (c_label, submenu);
      Interfaces.C.Strings.Free  (c_label);
   end glutAddSubMenu;

   procedure glutChangeToMenuEntry (item  : Integer;
                                    label : String;
                                    value : Integer) is
      c_label : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (label);
   begin
      glutChangeToMenuEntry  (item, c_label, value);
      Interfaces.C.Strings.Free  (c_label);
   end glutChangeToMenuEntry;

   procedure glutChangeToSubMenu (item    : Integer;
                                  label   : String;
                                  submenu : Integer) is
      c_label : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (label);
   begin
      glutChangeToSubMenu  (item, c_label, submenu);
      Interfaces.C.Strings.Free  (c_label);
   end glutChangeToSubMenu;

   function glutExtensionSupported (name : String) return Integer is
      Result : Integer;
      c_name : Interfaces.C.Strings.Chars_Ptr :=
         Interfaces.C.Strings.New_String (name);
   begin
      Result := glutExtensionSupported (c_name);
      Interfaces.C.Strings.Free  (c_name);
      return Result;
   end glutExtensionSupported;


end Glut;

