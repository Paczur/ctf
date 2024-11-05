#include "slice.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

uint32_t shttp_slice_length(shttp_slice s) {
  SHTTP_ASSERT(s.end >= s.begin);
  return s.end - s.begin;
}

bool shttp_slice_empty(shttp_slice s) { return s.end <= s.begin; }

shttp_status shttp_slice_parse_newline(shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin == '\r') begin++;
  if(begin == end) {
    s->begin = begin;
    return SHTTP_STATUS_SLICE_END;
  }
  if(*begin != '\n') {
    s->begin = begin;
    return SHTTP_STATUS_NEWLINE_EXPECTED;
  }
  begin++;
  s->begin = begin;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_space(shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict const begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin != ' ') return SHTTP_STATUS_SPACE_EXPECTED;
  s->begin++;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_space_optional(shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict const begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin == ' ') s->begin++;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_i32(int32_t out[static 1],
                                   shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  bool minus_flag = false;
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin == '-') {
    minus_flag = true;
    s->begin++;
    begin++;
  }
  if(*begin > '9' || *begin < '0') return SHTTP_STATUS_VALUE_INVALID;
  int32_t n = 0;
  while(begin != end && *begin <= '9' && *begin >= '0') {
    n *= 10;
    n += *begin - '0';
    begin++;
  }
  s->begin = begin;
  if(minus_flag) n = -n;
  *out = n;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_i16(int16_t out[static 1],
                                   shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  bool minus_flag = false;
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin == '-') {
    minus_flag = true;
    s->begin++;
    begin++;
  }
  if(*begin > '9' || *begin < '0') return SHTTP_STATUS_VALUE_INVALID;
  int16_t n = 0;
  while(begin != end && *begin <= '9' && *begin >= '0') {
    n *= 10;
    n += *begin - '0';
    begin++;
  }
  s->begin = begin;
  if(minus_flag) n = -n;
  *out = n;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_i8(int8_t out[static 1],
                                  shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  bool minus_flag = false;
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin == '-') {
    minus_flag = true;
    s->begin++;
    begin++;
  }
  if(*begin > '9' || *begin < '0') return SHTTP_STATUS_VALUE_INVALID;
  uint8_t n = 0;
  while(begin != end && *begin <= '9' && *begin >= '0') {
    n *= 10;
    n += *begin - '0';
    begin++;
  }
  s->begin = begin;
  if(minus_flag) n = -n;
  *out = n;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_u32(uint32_t out[static 1],
                                   shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin > '9' || *begin < '0') return SHTTP_STATUS_VALUE_INVALID;
  uint32_t n = 0;
  while(begin != end && *begin <= '9' && *begin >= '0') {
    n *= 10;
    n += *begin - '0';
    begin++;
  }
  s->begin = begin;
  *out = n;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_u16(uint16_t out[static 1],
                                   shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin > '9' || *begin < '0') return SHTTP_STATUS_VALUE_INVALID;
  uint16_t n = 0;
  while(begin != end && *begin <= '9' && *begin >= '0') {
    n *= 10;
    n += *begin - '0';
    begin++;
  }
  s->begin = begin;
  *out = n;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_u8(uint8_t out[static 1],
                                  shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(*begin > '9' || *begin < '0') return SHTTP_STATUS_VALUE_INVALID;
  uint8_t n = 0;
  while(begin != end && *begin <= '9' && *begin >= '0') {
    n *= 10;
    n += *begin - '0';
    begin++;
  }
  s->begin = begin;
  *out = n;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_parse_format(shttp_slice s[static 1],
                                      shttp_slice format, ...) {
  shttp_slice format_region = {.begin = format.begin};
  shttp_slice s_region = {.begin = s->begin};
  shttp_slice s_copy = *s;
  shttp_mut_slice stmp;
  shttp_status status = SHTTP_STATUS_OK;
  va_list arg_list;
  va_start(arg_list, format);

  while(true) {
    // Go to the special expr
    while(true) {
      if(shttp_slice_skip_until(&format, '{')) {
        // Format ended, still need to check before
      }

      // Compare all normal text found until now
      format_region.end = format.begin;
      s_region.end = s_region.begin + shttp_slice_length(format_region);
      if(s_region.end > s_copy.end) {
        status = SHTTP_STATUS_SLICE_END;
        goto cleanup;
      }
      if(!shttp_slice_eq(format_region, s_region)) {
        status = SHTTP_STATUS_VALUE_INVALID;
        goto cleanup;
      }
      s_copy.begin = s_region.end;
      if(format.begin == format.end) goto cleanup;
      if((status = shttp_slice_skip(&format, 1))) goto cleanup;
      if(*format.begin == '{') {
        if(*s_copy.begin != '{') {
          status = SHTTP_STATUS_FORMAT_INVALID;
          goto cleanup;
        }
        if((status = shttp_slice_skip(&s_copy, 1))) goto cleanup;
        if((status = shttp_slice_skip(&format, 1))) goto cleanup;
        format_region.begin = format.begin;
        s_region.begin = s_copy.begin;
      } else {
        format_region.begin = format.begin;
        s_region.begin = s_copy.begin;
        break;
      }
    }

    // Go to the end of special expr
    if(shttp_slice_skip_past(&format, '}')) {
      status = SHTTP_STATUS_FORMAT_INVALID;
      goto cleanup;
    }
    if(format.begin == format.end) {
      s_region.end = s_copy.end;
    } else {
      if(shttp_slice_skip_until(&s_copy, *format.begin)) {
        status = SHTTP_STATUS_VALUE_INVALID;
        goto cleanup;
      }
      s_region.end = s_copy.begin;
    }

    // Parse special expr
    format_region.end = format.begin - 1;
    switch(*format_region.begin) {
    case 's':
      format_region.begin++;
      if(!shttp_slice_eq(format_region, SHTTP_SLICE("lice"))) {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      stmp = va_arg(arg_list, shttp_mut_slice);
      if((status = shttp_slice_cpy(&stmp, s_region))) goto cleanup;
      break;
    case 'u':
      format_region.begin++;
      if(shttp_slice_eq(format_region, SHTTP_SLICE("8"))) {
        if((status =
              shttp_slice_parse_u8(va_arg(arg_list, uint8_t *), &s_region)))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("16"))) {
        if((status =
              shttp_slice_parse_u16(va_arg(arg_list, uint16_t *), &s_region)))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status =
              shttp_slice_parse_u32(va_arg(arg_list, uint32_t *), &s_region)))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    case 'i':
      format_region.begin++;
      if((shttp_slice_eq(format_region, SHTTP_SLICE("8")))) {
        if((status =
              shttp_slice_parse_i8(va_arg(arg_list, int8_t *), &s_region)))
          goto cleanup;
      } else if((shttp_slice_eq(format_region, SHTTP_SLICE("16")))) {
        if((status =
              shttp_slice_parse_i16(va_arg(arg_list, int16_t *), &s_region)))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status =
              shttp_slice_parse_i32(va_arg(arg_list, int32_t *), &s_region)))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    default:
      status = SHTTP_STATUS_FORMAT_INVALID;
      goto cleanup;
    }
    format_region.begin = format.begin;
    s_copy.begin = s_region.end;
    s_region.begin = s_copy.begin;
  }

cleanup:
  va_end(arg_list);
  if(status == SHTTP_STATUS_OK) *s = s_copy;
  return status;
}

shttp_status shttp_slice_parse_format_delim(shttp_slice s[static 1],
                                            shttp_slice format, char open,
                                            char close, ...) {
  shttp_slice format_region = {.begin = format.begin};
  shttp_slice s_region = {.begin = s->begin};
  shttp_slice s_copy = *s;
  shttp_mut_slice stmp;
  shttp_status status = SHTTP_STATUS_OK;
  va_list arg_list;
  va_start(arg_list, close);

  while(true) {
    // Go to the special expr
    while(true) {
      if(shttp_slice_skip_until(&format, open)) {
        // Format ended, still need to check before
      }

      // Compare all normal text found until now
      format_region.end = format.begin;
      s_region.end = s_region.begin + shttp_slice_length(format_region);
      if(s_region.end > s_copy.end) {
        status = SHTTP_STATUS_SLICE_END;
        goto cleanup;
      }
      if(!shttp_slice_eq(format_region, s_region)) {
        status = SHTTP_STATUS_VALUE_INVALID;
        goto cleanup;
      }
      s_copy.begin = s_region.end;
      if(format.begin == format.end) goto cleanup;
      if((status = shttp_slice_skip(&format, 1))) goto cleanup;
      if(*format.begin == open) {
        if(*s_copy.begin != open) {
          status = SHTTP_STATUS_FORMAT_INVALID;
          goto cleanup;
        }
        if((status = shttp_slice_skip(&s_copy, 1))) goto cleanup;
        if((status = shttp_slice_skip(&format, 1))) goto cleanup;
        format_region.begin = format.begin;
        s_region.begin = s_copy.begin;
      } else {
        format_region.begin = format.begin;
        s_region.begin = s_copy.begin;
        break;
      }
    }

    // Go to the end of special expr
    if(shttp_slice_skip_past(&format, close)) {
      status = SHTTP_STATUS_FORMAT_INVALID;
      goto cleanup;
    }
    if(format.begin == format.end) {
      s_region.end = s_copy.end;
    } else {
      if(shttp_slice_skip_until(&s_copy, *format.begin)) {
        status = SHTTP_STATUS_VALUE_INVALID;
        goto cleanup;
      }
      s_region.end = s_copy.begin;
    }

    // Parse special expr
    format_region.end = format.begin - 1;
    switch(*format_region.begin) {
    case 's':
      format_region.begin++;
      if(!shttp_slice_eq(format_region, SHTTP_SLICE("lice"))) {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      stmp = va_arg(arg_list, shttp_mut_slice);
      if((status = shttp_slice_cpy(&stmp, s_region))) goto cleanup;
      break;
    case 'u':
      format_region.begin++;
      if(shttp_slice_eq(format_region, SHTTP_SLICE("8"))) {
        if((status =
              shttp_slice_parse_u8(va_arg(arg_list, uint8_t *), &s_region)))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("16"))) {
        if((status =
              shttp_slice_parse_u16(va_arg(arg_list, uint16_t *), &s_region)))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status =
              shttp_slice_parse_u32(va_arg(arg_list, uint32_t *), &s_region)))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    case 'i':
      format_region.begin++;
      if((shttp_slice_eq(format_region, SHTTP_SLICE("8")))) {
        if((status =
              shttp_slice_parse_i8(va_arg(arg_list, int8_t *), &s_region)))
          goto cleanup;
      } else if((shttp_slice_eq(format_region, SHTTP_SLICE("16")))) {
        if((status =
              shttp_slice_parse_i16(va_arg(arg_list, int16_t *), &s_region)))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status =
              shttp_slice_parse_i32(va_arg(arg_list, int32_t *), &s_region)))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    default:
      status = SHTTP_STATUS_FORMAT_INVALID;
      goto cleanup;
    }
    format_region.begin = format.begin;
    s_copy.begin = s_region.end;
    s_region.begin = s_copy.begin;
  }

cleanup:
  va_end(arg_list);
  if(status == SHTTP_STATUS_OK) *s = s_copy;
  return status;
}

shttp_status shttp_slice_skip(shttp_slice s[static 1], uint32_t x) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict const begin = s->begin;
  const char *restrict const end = s->end;
  if(begin + x > end) return SHTTP_STATUS_SLICE_END;
  s->begin += x;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_skip_until(shttp_slice s[static 1], char mark) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  while(begin != end) {
    if(*begin == mark) {
      s->begin = begin;
      return SHTTP_STATUS_OK;
    }
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_SLICE_END;
}

shttp_status shttp_slice_skip_until_or(shttp_slice s[static 1],
                                       char marks[static 3]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  while(begin != end) {
    for(const char *mr = marks; *mr != 0; mr++) {
      if(*mr == *begin) {
        s->begin = begin;
        return SHTTP_STATUS_OK;
      }
    }
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_SLICE_END;
}

shttp_status shttp_slice_skip_until_newline(shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  shttp_status status;
  const char *const t = s->begin;
  if((status = shttp_slice_skip_until(s, '\n')))
    return SHTTP_STATUS_NEWLINE_EXPECTED;
  const char *const begin = s->begin;
  if(begin - t > 0 && begin[-1] == '\r') {
    s->begin--;
  }
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_skip_until_space(shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  shttp_status status;
  if((status = shttp_slice_skip_until(s, ' ')))
    return SHTTP_STATUS_SPACE_EXPECTED;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_skip_past(shttp_slice s[static 1], char end) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  SHTTP_PROP(shttp_slice_skip_until(s, end));
  s->begin++;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_skip_past_or(shttp_slice s[static 1],
                                      char end[static 3]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(end != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  SHTTP_PROP(shttp_slice_skip_until_or(s, end));
  s->begin++;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_skip_past_newline(shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  shttp_status status;
  if((status = shttp_slice_skip_past(s, '\n')))
    return SHTTP_STATUS_NEWLINE_EXPECTED;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_skip_past_space(shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  shttp_status status;
  if((status = shttp_slice_skip_past(s, ' ')))
    return SHTTP_STATUS_SPACE_EXPECTED;
  return SHTTP_STATUS_OK;
}

bool shttp_slice_starts_with(shttp_slice s1, shttp_slice s2) {
  SHTTP_ASSERT(s1.begin <= s1.end);
  SHTTP_ASSERT(s2.begin <= s2.end);
  const char *restrict begin1 = s1.begin;
  const char *restrict begin2 = s2.begin;
  const char *restrict const end2 = s2.end;
  if(s1.end - begin1 < end2 - begin2) return false;
  while(begin2 != end2) {
    if(*begin1 != *begin2) return false;
    begin1++;
    begin2++;
  }
  return true;
}

bool shttp_slice_eq(shttp_slice s1, shttp_slice s2) {
  SHTTP_ASSERT(s1.begin <= s1.end);
  SHTTP_ASSERT(s2.begin <= s2.end);
  const char *restrict begin1 = s1.begin;
  const char *restrict begin2 = s2.begin;
  const char *restrict const end2 = s2.end;
  if(shttp_slice_length(s1) != shttp_slice_length(s2)) return false;
  while(begin2 != end2) {
    if(*begin1 != *begin2) return false;
    begin1++;
    begin2++;
  }
  return true;
}

bool shttp_slice_eq_until(shttp_slice s1, shttp_slice s2, char mark) {
  SHTTP_ASSERT(s2.begin <= s2.end);
  SHTTP_ASSERT(s1.begin <= s1.end);
  const char *restrict begin1 = s1.begin;
  const char *restrict const end1 = s1.end;
  const char *restrict begin2 = s2.begin;
  const char *restrict const end2 = s2.end;
  if(end1 - begin1 < end2 - begin2) {
    while(begin1 != end1 && *begin1 != mark && *begin2 != mark) {
      if(*begin1 != *begin2) return false;
      begin2++;
      begin1++;
    }
  } else {
    while(begin2 != end2 && *begin1 != mark && *begin2 != mark) {
      if(*begin1 != *begin2) return false;
      begin2++;
      begin1++;
    }
  }
  return true;
}

shttp_status shttp_header_next(shttp_slice out[static 1],
                               shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(out->begin <= out->end);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict const begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  out->begin = begin;
  SHTTP_PROP(shttp_slice_skip_until_newline(s));
  out->end = s->begin;
  return shttp_slice_parse_newline(s);
}

shttp_status shttp_header_key(shttp_slice out[static 1],
                              shttp_slice s[static 1]) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(out->begin <= out->end);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *restrict const begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  out->begin = begin;
  SHTTP_PROP(shttp_slice_skip_until(s, ':'));
  out->end = s->begin;
  s->begin++;
  return shttp_slice_parse_space_optional(s);
}

shttp_status shttp_header_value(shttp_slice out[static 1],
                                shttp_slice s[static 1]) {
  SHTTP_ASSERT(out != NULL);
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(out->begin <= out->end);
  SHTTP_ASSERT(s->begin <= s->end);
  const char *t;
  shttp_status status;
  const char *restrict begin = s->begin;
  const char *restrict const end = s->end;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  out->begin = begin;
  t = begin;
  SHTTP_PROP(shttp_slice_skip_until_or(s, " \n,"));
  begin = s->begin;
  if(begin - t > 0 && begin[-1] == '\r') {
    out->end = begin - 1;
  } else {
    out->end = begin;
  }
  if(*begin == ',' || *begin == '\n') begin++;
  s->begin = begin;
  if((status = shttp_slice_parse_space_optional(s)) != SHTTP_STATUS_SLICE_END)
    return status;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_newline(shttp_mut_slice msg[static 1]) {
  SHTTP_ASSERT(msg != NULL);
  SHTTP_ASSERT(msg->begin <= msg->end);
  char *restrict beginm = msg->begin;
  char *restrict const endm = msg->end;
  if(beginm == endm) return SHTTP_STATUS_SLICE_END;
  *(beginm++) = '\r';
  if(beginm == endm) {
    msg->begin = beginm;
    return SHTTP_STATUS_SLICE_END;
  }
  *beginm = '\n';
  msg->begin = beginm + 1;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_u32(shttp_mut_slice s[static 1], uint32_t val) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  char *restrict begin = s->begin;
  char *restrict const end = s->end;
  uint32_t mag = 10;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  while(mag <= val) mag *= 10;
  mag /= 10;
  *begin = val / mag + '0';
  val %= mag;
  mag /= 10;
  begin++;
  while(mag != 0) {
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
    *begin = val / mag + '0';
    val %= mag;
    mag /= 10;
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_u16(shttp_mut_slice s[static 1], uint16_t val) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  char *restrict begin = s->begin;
  char *restrict const end = s->end;
  uint32_t mag = 10;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  while(mag <= val) mag *= 10;
  mag /= 10;
  *begin = val / mag + '0';
  val %= mag;
  mag /= 10;
  begin++;
  while(mag != 0) {
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
    *begin = val / mag + '0';
    val %= mag;
    mag /= 10;
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_u8(shttp_mut_slice s[static 1], uint8_t val) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  char *restrict begin = s->begin;
  char *restrict const end = s->end;
  uint32_t mag = 10;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  while(mag <= val) mag *= 10;
  mag /= 10;
  *begin = val / mag + '0';
  val %= mag;
  mag /= 10;
  begin++;
  while(mag != 0) {
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
    *begin = val / mag + '0';
    val %= mag;
    mag /= 10;
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_i32(shttp_mut_slice s[static 1], int32_t val) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  char *restrict begin = s->begin;
  char *restrict const end = s->end;
  int32_t mag = 10;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(val < 0) {
    val = -val;
    *begin = '-';
    begin++;
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
  }
  while(mag <= val) mag *= 10;
  mag /= 10;
  *begin = val / mag + '0';
  val %= mag;
  mag /= 10;
  begin++;
  while(mag != 0) {
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
    *begin = val / mag + '0';
    val %= mag;
    mag /= 10;
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_i16(shttp_mut_slice s[static 1], int16_t val) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  char *restrict begin = s->begin;
  char *restrict const end = s->end;
  int32_t mag = 10;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(val < 0) {
    val = -val;
    *begin = '-';
    begin++;
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
  }
  while(mag <= val) mag *= 10;
  mag /= 10;
  *begin = val / mag + '0';
  val %= mag;
  mag /= 10;
  begin++;
  while(mag != 0) {
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
    *begin = val / mag + '0';
    val %= mag;
    mag /= 10;
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_i8(shttp_mut_slice s[static 1], int8_t val) {
  SHTTP_ASSERT(s != NULL);
  SHTTP_ASSERT(s->begin <= s->end);
  char *restrict begin = s->begin;
  char *restrict const end = s->end;
  int32_t mag = 10;
  if(begin == end) return SHTTP_STATUS_SLICE_END;
  if(val < 0) {
    val = -val;
    *begin = '-';
    begin++;
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
  }
  while(mag <= val) mag *= 10;
  mag /= 10;
  *begin = val / mag + '0';
  val %= mag;
  mag /= 10;
  begin++;
  while(mag != 0) {
    if(begin == end) {
      s->begin = begin;
      return SHTTP_STATUS_SLICE_END;
    }
    *begin = val / mag + '0';
    val %= mag;
    mag /= 10;
    begin++;
  }
  s->begin = begin;
  return SHTTP_STATUS_OK;
}

shttp_status shttp_slice_insert_format(shttp_mut_slice s[static 1],
                                       shttp_slice format, ...) {
  shttp_slice format_region = {.begin = format.begin, .end = format.begin};
  shttp_status status = SHTTP_STATUS_OK;
  va_list arg_list;
  va_start(arg_list, format);

  while(true) {
    while(true) {
      if(shttp_slice_skip_until(&format, '{')) {
      }
      format_region.end = format.begin;
      if((status = shttp_slice_cpy(s, format_region))) goto cleanup;
      if(format.begin == format.end) goto cleanup;
      if((status = shttp_slice_skip(&format, 1))) goto cleanup;
      if(*format.begin == '{') {
        if((status = shttp_slice_cpy_char(s, '{'))) goto cleanup;
        if((status = shttp_slice_skip(&format, 1))) goto cleanup;
        format_region.begin = format.begin;
      } else {
        format_region.begin = format.begin;
        break;
      }
    }
    if((status = shttp_slice_skip_past(&format, '}'))) goto cleanup;
    format_region.end = format.begin - 1;

    switch(*format_region.begin) {
    case 's':
      format_region.begin++;
      if(!shttp_slice_eq(format_region, SHTTP_SLICE("lice"))) {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      if((status = shttp_slice_cpy(s, va_arg(arg_list, shttp_slice))))
        goto cleanup;
      break;
    case 'u':
      format_region.begin++;
      if(shttp_slice_eq(format_region, SHTTP_SLICE("8"))) {
        if((status = shttp_slice_insert_u8(s, (uint8_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("16"))) {
        if((status =
              shttp_slice_insert_u16(s, (uint16_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status =
              shttp_slice_insert_u32(s, (uint32_t)va_arg(arg_list, int))))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    case 'i':
      format_region.begin++;
      if(shttp_slice_eq(format_region, SHTTP_SLICE("8"))) {
        if((status = shttp_slice_insert_i8(s, (int8_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("16"))) {
        if((status = shttp_slice_insert_i16(s, (int16_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status = shttp_slice_insert_i32(s, (int32_t)va_arg(arg_list, int))))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    default:
      status = SHTTP_STATUS_FORMAT_INVALID;
      goto cleanup;
    }
    format_region.begin = format.begin;
  }

cleanup:
  va_end(arg_list);
  return status;
}

shttp_status shttp_slice_insert_format_delim(shttp_mut_slice s[static 1],
                                             shttp_slice format, char open,
                                             char close, ...) {
  shttp_slice format_region = {.begin = format.begin, .end = format.begin};
  shttp_status status = SHTTP_STATUS_OK;
  va_list arg_list;
  va_start(arg_list, close);

  while(true) {
    while(true) {
      if(shttp_slice_skip_until(&format, open)) {
      }
      format_region.end = format.begin;
      if((status = shttp_slice_cpy(s, format_region))) goto cleanup;
      if(format.begin == format.end) goto cleanup;
      if((status = shttp_slice_skip(&format, 1))) goto cleanup;
      if(*format.begin == open) {
        if((status = shttp_slice_cpy_char(s, open))) goto cleanup;
        if((status = shttp_slice_skip(&format, 1))) goto cleanup;
        format_region.begin = format.begin;
      } else {
        format_region.begin = format.begin;
        break;
      }
    }
    if((status = shttp_slice_skip_past(&format, close))) goto cleanup;
    format_region.end = format.begin - 1;

    switch(*format_region.begin) {
    case 's':
      format_region.begin++;
      if(!shttp_slice_eq(format_region, SHTTP_SLICE("lice"))) {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      if((status = shttp_slice_cpy(s, va_arg(arg_list, shttp_slice))))
        goto cleanup;
      break;
    case 'u':
      format_region.begin++;
      if(shttp_slice_eq(format_region, SHTTP_SLICE("8"))) {
        if((status = shttp_slice_insert_u8(s, (uint8_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("16"))) {
        if((status =
              shttp_slice_insert_u16(s, (uint16_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status =
              shttp_slice_insert_u32(s, (uint32_t)va_arg(arg_list, int))))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    case 'i':
      format_region.begin++;
      if(shttp_slice_eq(format_region, SHTTP_SLICE("8"))) {
        if((status = shttp_slice_insert_i8(s, (int8_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("16"))) {
        if((status = shttp_slice_insert_i16(s, (int16_t)va_arg(arg_list, int))))
          goto cleanup;
      } else if(shttp_slice_eq(format_region, SHTTP_SLICE("32"))) {
        if((status = shttp_slice_insert_i32(s, (int32_t)va_arg(arg_list, int))))
          goto cleanup;
      } else {
        status = SHTTP_STATUS_FORMAT_INVALID;
        goto cleanup;
      }
      break;
    default:
      status = SHTTP_STATUS_FORMAT_INVALID;
      goto cleanup;
    }
    format_region.begin = format.begin;
  }

cleanup:
  va_end(arg_list);
  return status;
}

shttp_status shttp_slice_cpy(shttp_mut_slice msg[static 1], shttp_slice slice) {
  SHTTP_ASSERT(msg != NULL);
  SHTTP_ASSERT(msg->begin <= msg->end);
  SHTTP_ASSERT(slice.begin <= slice.end);
  char *restrict beginm = msg->begin;
  char *restrict const endm = msg->end;
  const char *restrict begint = slice.begin;
  const char *restrict const endt = slice.end;
  if(endm == beginm && endt != begint) return SHTTP_STATUS_SLICE_END;

  if(endm - beginm < endt - begint) {
    while(beginm != endm) {
      *beginm = *begint;
      beginm++;
      begint++;
    }
    msg->begin = beginm;
    return SHTTP_STATUS_SLICE_END;
  } else {
    while(begint != endt) {
      *beginm = *begint;
      beginm++;
      begint++;
    }
    msg->begin = beginm;
    return SHTTP_STATUS_OK;
  }
}

shttp_status shttp_slice_cpy_char(shttp_mut_slice msg[static 1], char c) {
  SHTTP_ASSERT(msg != NULL);
  SHTTP_ASSERT(msg->begin <= msg->end);
  char *restrict beginm = msg->begin;
  char *restrict const endm = msg->end;
  if(endm == beginm) return SHTTP_STATUS_SLICE_END;
  *beginm = c;
  msg->begin++;
  return SHTTP_STATUS_OK;
}
