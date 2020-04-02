/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/notify.h>

static u32_t get_extflags(const struct sys_notify *anp)
{
	u32_t flags = anp->flags & SYS_NOTIFY_EXTENSION_MASK;

	return flags >> SYS_NOTIFY_EXTENSION_POS;
}

static void set_extflags(struct sys_notify *anp,
			 u32_t flags)
{
	anp->flags = (anp->flags & ~SYS_NOTIFY_EXTENSION_MASK)
		     | (flags << SYS_NOTIFY_EXTENSION_POS);
}

static void callback(struct sys_notify *anp,
		     int *resp)
{
	zassert_equal(sys_notify_fetch_result(anp, resp), 0,
		      "failed callback fetch");
}

static void test_validate(void)
{
	struct sys_notify notify = {
		.flags = 0,
	};

	zassert_equal(sys_notify_validate(NULL), -EINVAL,
		      "accepted null pointer");
	zassert_equal(sys_notify_validate(&notify), -EINVAL,
		      "accepted bad method");
}


static void test_spinwait(void)
{
	int rc;
	int set_res = 423;
	int res;
	sys_notify_generic_callback cb;
	struct sys_notify notify;
	u32_t xflags = 0x1234;

	memset(&notify, 0xac, sizeof(notify));
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, -EINVAL,
		      "invalid not diagnosed");

	sys_notify_init_spinwait(&notify);
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, 0,
		      "init_spinwait invalid");

	zassert_false(sys_notify_uses_callback(&notify),
		      "uses callback");

	zassert_equal(notify.flags, SYS_NOTIFY_METHOD_SPINWAIT,
		      "flags mismatch");

	set_extflags(&notify, xflags);
	zassert_equal(sys_notify_get_method(&notify),
		      SYS_NOTIFY_METHOD_SPINWAIT,
		      "method corrupted");
	zassert_equal(get_extflags(&notify), xflags,
		      "xflags extract failed");

	rc = sys_notify_fetch_result(&notify, &res);
	zassert_equal(rc, -EAGAIN,
		      "spinwait ready too soon");

	zassert_not_equal(notify.flags, 0,
			  "flags cleared");

	cb = sys_notify_finalize(&notify, set_res);
	zassert_equal(cb, (sys_notify_generic_callback)NULL,
		      "callback not null");
	zassert_equal(notify.flags, 0,
		      "flags not cleared");

	rc = sys_notify_fetch_result(&notify, &res);
	zassert_equal(rc, 0,
		      "spinwait not ready");
	zassert_equal(res, set_res,
		      "result not set");
}

static void test_signal(void)
{
#ifdef CONFIG_POLL
	int rc;
	int set_res = 423;
	int res;
	struct k_poll_signal sig;
	sys_notify_generic_callback cb;
	struct sys_notify notify;
	u32_t xflags = 0x1234;

	memset(&notify, 0xac, sizeof(notify));
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, -EINVAL,
		      "invalid not diagnosed");

	k_poll_signal_init(&sig);
	k_poll_signal_check(&sig, &rc, &res);
	zassert_equal(rc, 0,
		      "signal set");

	sys_notify_init_signal(&notify, &sig);
	notify.method.signal = NULL;
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, -EINVAL,
		      "null signal not invalid");

	memset(&notify, 0xac, sizeof(notify));
	sys_notify_init_signal(&notify, &sig);
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, 0,
		      "init_spinwait invalid");

	zassert_false(sys_notify_uses_callback(&notify),
		      "uses callback");

	zassert_equal(notify.flags, SYS_NOTIFY_METHOD_SIGNAL,
		      "flags mismatch");
	zassert_equal(notify.method.signal, &sig,
		      "signal pointer mismatch");

	set_extflags(&notify, xflags);
	zassert_equal(sys_notify_get_method(&notify),
		      SYS_NOTIFY_METHOD_SIGNAL,
		      "method corrupted");
	zassert_equal(get_extflags(&notify), xflags,
		      "xflags extract failed");

	rc = sys_notify_fetch_result(&notify, &res);
	zassert_equal(rc, -EAGAIN,
		      "spinwait ready too soon");

	zassert_not_equal(notify.flags, 0,
			  "flags cleared");

	cb = sys_notify_finalize(&notify, set_res);
	zassert_equal(cb, (sys_notify_generic_callback)NULL,
		      "callback not null");
	zassert_equal(notify.flags, 0,
		      "flags not cleared");
	k_poll_signal_check(&sig, &rc, &res);
	zassert_equal(rc, 1,
		      "signal not set");
	zassert_equal(res, set_res,
		      "signal result wrong");

	rc = sys_notify_fetch_result(&notify, &res);
	zassert_equal(rc, 0,
		      "signal not ready");
	zassert_equal(res, set_res,
		      "result not set");
#endif /* CONFIG_POLL */
}

static void test_callback(void)
{
	int rc;
	int set_res = 423;
	int res;
	sys_notify_generic_callback cb;
	struct sys_notify notify;
	u32_t xflags = 0x8765432;

	memset(&notify, 0xac, sizeof(notify));
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, -EINVAL,
		      "invalid not diagnosed");

	sys_notify_init_callback(&notify, callback);
	notify.method.callback = NULL;
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, -EINVAL,
		      "null callback not invalid");

	memset(&notify, 0xac, sizeof(notify));
	sys_notify_init_callback(&notify, callback);
	rc = sys_notify_validate(&notify);
	zassert_equal(rc, 0,
		      "init_spinwait invalid");

	zassert_true(sys_notify_uses_callback(&notify),
		     "not using callback");

	zassert_equal(notify.flags, SYS_NOTIFY_METHOD_CALLBACK,
		      "flags mismatch");
	zassert_equal(notify.method.callback,
		      (sys_notify_generic_callback)callback,
		      "callback mismatch");

	set_extflags(&notify, xflags);
	zassert_equal(sys_notify_get_method(&notify),
		      SYS_NOTIFY_METHOD_CALLBACK,
		      "method corrupted");
	zassert_equal(get_extflags(&notify), xflags,
		      "xflags extract failed");

	rc = sys_notify_fetch_result(&notify, &res);
	zassert_equal(rc, -EAGAIN,
		      "callback ready too soon");

	zassert_not_equal(notify.flags, 0,
			  "flags cleared");

	cb = sys_notify_finalize(&notify, set_res);
	zassert_equal(cb, (sys_notify_generic_callback)callback,
		      "callback wrong");
	zassert_equal(notify.flags, 0,
		      "flags not cleared");

	res = ~set_res;
	((sys_notify_generic_callback)cb)(&notify, &res);
	zassert_equal(res, set_res,
		      "result not set");
}

void test_main(void)
{
	ztest_test_suite(sys_notify_api,
			 ztest_unit_test(test_validate),
			 ztest_unit_test(test_spinwait),
			 ztest_unit_test(test_signal),
			 ztest_unit_test(test_callback));
	ztest_run_test_suite(sys_notify_api);
}