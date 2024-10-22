#!/bin/awk -f
# SPDX-License-Identifier: GPL-2.0
# gen-cpu-sysreg-properties.awk: arm64 sysreg header generator
#
# Usage: awk -f gen-cpu-sysreg-properties.awk $LINUX_PATH/arch/arm64/tools/sysreg

function block_current() {
	return __current_block[__current_block_depth];
}

# Log an error and terminate
function fatal(msg) {
	print "Error at " NR ": " msg > "/dev/stderr"

	printf "Current block nesting:"

	for (i = 0; i <= __current_block_depth; i++) {
		printf " " __current_block[i]
	}
	printf "\n"

	exit 1
}

# Enter a new block, setting the active block to @block
function block_push(block) {
	__current_block[++__current_block_depth] = block
}

# Exit a block, setting the active block to the parent block
function block_pop() {
	if (__current_block_depth == 0)
		fatal("error: block_pop() in root block")

	__current_block_depth--;
}

# Sanity check the number of records for a field makes sense. If not, produce
# an error and terminate.
function expect_fields(nf) {
	if (NF != nf)
		fatal(NF " fields found where " nf " expected")
}

# Print a CPP macro definition, padded with spaces so that the macro bodies
# line up in a column
function define(name, val) {
	printf "%-56s%s\n", "#define " name, val
}

# Print standard BITMASK/SHIFT/WIDTH CPP definitions for a field
function define_field(reg, field, msb, lsb, idreg) {
	if (idreg)
            print "    arm64_sysreg_add_field("reg", \""field"\", "lsb", "msb");"
}

# Print a field _SIGNED definition for a field
function define_field_sign(reg, field, sign, idreg) {
	if (idreg)
            print "    arm64_sysreg_add_field("reg", \""field"\", "lsb", "msb");"
}

# Parse a "<msb>[:<lsb>]" string into the global variables @msb and @lsb
function parse_bitdef(reg, field, bitdef, _bits)
{
	if (bitdef ~ /^[0-9]+$/) {
		msb = bitdef
		lsb = bitdef
	} else if (split(bitdef, _bits, ":") == 2) {
		msb = _bits[1]
		lsb = _bits[2]
	} else {
		fatal("invalid bit-range definition '" bitdef "'")
	}


	if (msb != next_bit)
		fatal(reg "." field " starts at " msb " not " next_bit)
	if (63 < msb || msb < 0)
		fatal(reg "." field " invalid high bit in '" bitdef "'")
	if (63 < lsb || lsb < 0)
		fatal(reg "." field " invalid low bit in '" bitdef "'")
	if (msb < lsb)
		fatal(reg "." field " invalid bit-range '" bitdef "'")
	if (low > high)
		fatal(reg "." field " has invalid range " high "-" low)

	next_bit = lsb - 1
}

BEGIN {
	print "#include \"cpu-custom.h\""
	print ""
	print "ARM64SysReg arm64_id_regs[NUM_ID_IDX];"
	print ""
	print "void initialize_cpu_sysreg_properties(void)"
	print "{"
        print "    memset(arm64_id_regs, 0, sizeof(ARM64SysReg) * NUM_ID_IDX);"
        print ""

	__current_block_depth = 0
	__current_block[__current_block_depth] = "Root"
}

END {
	if (__current_block_depth != 0)
		fatal("Missing terminator for " block_current() " block")

	print "}"
}

# skip blank lines and comment lines
/^$/ { next }
/^[\t ]*#/ { next }

/^SysregFields/ && block_current() == "Root" {
	block_push("SysregFields")

	expect_fields(2)

	reg = $2

	res0 = "UL(0)"
	res1 = "UL(0)"
	unkn = "UL(0)"

	next_bit = 63

	next
}

/^EndSysregFields/ && block_current() == "SysregFields" {
	if (next_bit > 0)
		fatal("Unspecified bits in " reg)

	reg = null
	res0 = null
	res1 = null
	unkn = null

	block_pop()
	next
}

/^Sysreg/ && block_current() == "Root" {
	block_push("Sysreg")

	expect_fields(7)

	reg = $2
	op0 = $3
	op1 = $4
	crn = $5
	crm = $6
	op2 = $7

	res0 = "UL(0)"
	res1 = "UL(0)"
	unkn = "UL(0)"

	if (op0 == 3 && (op1>=0 && op1<=3) && crn==0 && (crm>=0 && crm<=7) && (op2>=0 && op2<=7)) {
	    idreg = 1
        } else {
	    idreg = 0
	}

	if (idreg == 1) {
	   print "    /* "reg" */"
	   print "    ARM64SysReg *"reg" = arm64_sysreg_get("reg"_IDX);"
	   print "    "reg"->name = \""reg"\";"
	}

	next_bit = 63

	next
}

/^EndSysreg/ && block_current() == "Sysreg" {
	if (next_bit > 0)
		fatal("Unspecified bits in " reg)

	reg = null
	op0 = null
	op1 = null
	crn = null
	crm = null
	op2 = null
	res0 = null
	res1 = null
	unkn = null

	if (idreg==1)
	    print ""
	block_pop()
	next
}

# Currently this is effectivey a comment, in future we may want to emit
# defines for the fields.
(/^Fields/ || /^Mapping/) && block_current() == "Sysreg" {
	expect_fields(2)

	if (next_bit != 63)
		fatal("Some fields already defined for " reg)

	print "/* For " reg " fields see " $2 " */"
	print ""

        next_bit = 0
	res0 = null
	res1 = null
	unkn = null

	next
}


/^Res0/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	expect_fields(2)
	parse_bitdef(reg, "RES0", $2)
	field = "RES0_" msb "_" lsb

	res0 = res0 " | GENMASK_ULL(" msb ", " lsb ")"

	next
}

/^Res1/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	expect_fields(2)
	parse_bitdef(reg, "RES1", $2)
	field = "RES1_" msb "_" lsb

	res1 = res1 " | GENMASK_ULL(" msb ", " lsb ")"

	next
}

/^Unkn/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	expect_fields(2)
	parse_bitdef(reg, "UNKN", $2)
	field = "UNKN_" msb "_" lsb

	unkn = unkn " | GENMASK_ULL(" msb ", " lsb ")"

	next
}

/^Field/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	expect_fields(3)
	field = $3
	parse_bitdef(reg, field, $2)


	define_field(reg, field, msb, lsb, idreg)

	next
}

/^Raz/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	expect_fields(2)
	parse_bitdef(reg, field, $2)

	next
}

/^SignedEnum/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	block_push("Enum")

	expect_fields(3)
	field = $3
	parse_bitdef(reg, field, $2)

	define_field(reg, field, msb, lsb, idreg)
	define_field_sign(reg, field, "true", idreg)

	next
}

/^UnsignedEnum/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	block_push("Enum")

	expect_fields(3)
	field = $3
	parse_bitdef(reg, field, $2)

	define_field(reg, field, msb, lsb, idreg)
	#define_field_sign(reg, field, "false", idreg)

	next
}

/^Enum/ && (block_current() == "Sysreg" || block_current() == "SysregFields") {
	block_push("Enum")

	expect_fields(3)
	field = $3
	parse_bitdef(reg, field, $2)

	define_field(reg, field, msb, lsb, idreg)

	next
}

/^EndEnum/ && block_current() == "Enum" {

	field = null
	msb = null
	lsb = null

	block_pop()
	next
}

/0b[01]+/ && block_current() == "Enum" {
	expect_fields(2)
	val = $1
	name = $2

	next
}

# Any lines not handled by previous rules are unexpected
{
	fatal("unhandled statement")
}
