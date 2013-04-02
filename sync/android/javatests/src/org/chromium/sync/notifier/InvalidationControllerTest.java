// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.sync.notifier;

import android.accounts.Account;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.google.common.collect.Lists;
import com.google.common.collect.Sets;

import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Feature;
import org.chromium.sync.internal_api.pub.base.ModelType;
import org.chromium.sync.notifier.InvalidationController.IntentProtocol;
import org.chromium.sync.signin.AccountManagerHelper;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Tests for the {@link InvalidationController}.
 */
public class InvalidationControllerTest extends InstrumentationTestCase {
    private IntentSavingContext mContext;
    private InvalidationController mController;

    @Override
    protected void setUp() throws Exception {
        mContext = new IntentSavingContext(getInstrumentation().getTargetContext());
        mController = InvalidationController.newInstance(mContext);
    }

    @SmallTest
    @Feature({"Sync"})
    public void testStart() throws Exception {
        mController.start();
        assertEquals(1, mContext.getNumStartedIntents());
        Intent intent = mContext.getStartedIntent(0);
        validateIntentComponent(intent);
        assertNull(intent.getExtras());
    }

    @SmallTest
    @Feature({"Sync"})
    public void testStop() throws Exception {
        mController.stop();
        assertEquals(1, mContext.getNumStartedIntents());
        Intent intent = mContext.getStartedIntent(0);
        validateIntentComponent(intent);
        assertEquals(1, intent.getExtras().size());
        assertTrue(intent.hasExtra(IntentProtocol.EXTRA_STOP));
        assertTrue(intent.getBooleanExtra(IntentProtocol.EXTRA_STOP, false));
    }

    @SmallTest
    @Feature({"Sync"})
    public void testRegisterForSpecificTypes() {
        final String controllerFlag = "resolveModelTypes";
        final ModelTypeResolver resolver = new ModelTypeResolver() {
            @Override
            public Set<ModelType> resolveModelTypes(Set<ModelType> modelTypes) {
                mContext.setFlag(controllerFlag);
                return modelTypes;
            }
        };
        InvalidationController controller = new InvalidationController(mContext) {
            @Override
            ModelTypeResolver getModelTypeResolver() {
                return resolver;
            }
        };
        Account account = new Account("test@example.com", "bogus");
        controller.setRegisteredTypes(account, false,
                Sets.newHashSet(ModelType.BOOKMARK, ModelType.SESSION));
        assertEquals(1, mContext.getNumStartedIntents());

        // Validate destination.
        Intent intent = mContext.getStartedIntent(0);
        validateIntentComponent(intent);
        assertEquals(IntentProtocol.ACTION_REGISTER, intent.getAction());

        // Validate account.
        Account intentAccount = intent.getParcelableExtra(IntentProtocol.EXTRA_ACCOUNT);
        assertEquals(account, intentAccount);

        // Validate registered types.
        Set<String> expectedTypes =
                Sets.newHashSet(ModelType.BOOKMARK.name(), ModelType.SESSION.name());
        Set<String> actualTypes = Sets.newHashSet();
        actualTypes.addAll(intent.getStringArrayListExtra(IntentProtocol.EXTRA_REGISTERED_TYPES));
        assertEquals(expectedTypes, actualTypes);
        assertTrue(mContext.isFlagSet(controllerFlag));
    }

    @SmallTest
    @Feature({"Sync"})
    public void testRegisterForAllTypes() {
        Account account = new Account("test@example.com", "bogus");
        mController.setRegisteredTypes(account, true,
                Sets.newHashSet(ModelType.BOOKMARK, ModelType.SESSION));
        assertEquals(1, mContext.getNumStartedIntents());

        // Validate destination.
        Intent intent = mContext.getStartedIntent(0);
        validateIntentComponent(intent);
        assertEquals(IntentProtocol.ACTION_REGISTER, intent.getAction());

        // Validate account.
        Account intentAccount = intent.getParcelableExtra(IntentProtocol.EXTRA_ACCOUNT);
        assertEquals(account, intentAccount);

        // Validate registered types.
        Set<String> expectedTypes = Sets.newHashSet(ModelType.ALL_TYPES_TYPE);
        Set<String> actualTypes = Sets.newHashSet();
        actualTypes.addAll(intent.getStringArrayListExtra(IntentProtocol.EXTRA_REGISTERED_TYPES));
        assertEquals(expectedTypes, actualTypes);
    }

    @SmallTest
    @Feature({"Sync"})
    public void testRefreshShouldReadValuesFromDiskWithSpecificTypes() {
        // Store some preferences for ModelTypes and account. We are using the helper class
        // for this, so we don't have to deal with low-level details such as preference keys.
        InvalidationPreferences invalidationPreferences = new InvalidationPreferences(mContext);
        InvalidationPreferences.EditContext edit = invalidationPreferences.edit();
        Set<String> storedModelTypes = new HashSet<String>();
        storedModelTypes.add(ModelType.BOOKMARK.name());
        storedModelTypes.add(ModelType.TYPED_URL.name());
        invalidationPreferences.setSyncTypes(edit, storedModelTypes);
        Account storedAccount = AccountManagerHelper.createAccountFromName("test@gmail.com");
        invalidationPreferences.setAccount(edit, storedAccount);
        invalidationPreferences.commit(edit);

        // Ensure all calls to {@link InvalidationController#setRegisteredTypes} store values
        // we can inspect in the test.
        final AtomicReference<Account> resultAccount = new AtomicReference<Account>();
        final AtomicBoolean resultAllTypes = new AtomicBoolean();
        final AtomicReference<Set<ModelType>> resultTypes = new AtomicReference<Set<ModelType>>();
        InvalidationController controller = new InvalidationController(mContext) {
            @Override
            public void setRegisteredTypes(
                    Account account, boolean allTypes, Set<ModelType> types) {
                resultAccount.set(account);
                resultAllTypes.set(allTypes);
                resultTypes.set(types);
            }
        };

        // Execute the test.
        controller.refreshRegisteredTypes();

        // Validate the values.
        assertEquals(storedAccount, resultAccount.get());
        assertEquals(false, resultAllTypes.get());
        assertEquals(ModelType.syncTypesToModelTypes(storedModelTypes), resultTypes.get());
    }

    @SmallTest
    @Feature({"Sync"})
    public void testRefreshShouldReadValuesFromDiskWithAllTypes() {
        // Store preferences for the ModelType.ALL_TYPES_TYPE and account. We are using the
        // helper class for this, so we don't have to deal with low-level details such as preference
        // keys.
        InvalidationPreferences invalidationPreferences = new InvalidationPreferences(mContext);
        InvalidationPreferences.EditContext edit = invalidationPreferences.edit();
        List<String> storedModelTypes = new ArrayList<String>();
        storedModelTypes.add(ModelType.ALL_TYPES_TYPE);
        invalidationPreferences.setSyncTypes(edit, storedModelTypes);
        Account storedAccount = AccountManagerHelper.createAccountFromName("test@gmail.com");
        invalidationPreferences.setAccount(edit, storedAccount);
        invalidationPreferences.commit(edit);

        // Ensure all calls to {@link InvalidationController#setRegisteredTypes} store values
        // we can inspect in the test.
        final AtomicReference<Account> resultAccount = new AtomicReference<Account>();
        final AtomicBoolean resultAllTypes = new AtomicBoolean();
        final AtomicReference<Set<ModelType>> resultTypes = new AtomicReference<Set<ModelType>>();
        InvalidationController controller = new InvalidationController(mContext) {
            @Override
            public void setRegisteredTypes(
                    Account account, boolean allTypes, Set<ModelType> types) {
                resultAccount.set(account);
                resultAllTypes.set(allTypes);
                resultTypes.set(types);
            }
        };

        // Execute the test.
        controller.refreshRegisteredTypes();

        // Validate the values.
        assertEquals(storedAccount, resultAccount.get());
        assertEquals(true, resultAllTypes.get());
    }

    @SmallTest
    @Feature({"Sync"})
    public void testGetContractAuthority() throws Exception {
        assertEquals(mContext.getPackageName(), mController.getContractAuthority());
    }

    @SmallTest
    @Feature({"Sync"})
    public void testGetIntentDestination() {
        assertEquals("org.chromium.sync.notifier.TEST_VALUE",
                InvalidationController.getDestinationClassName(mContext));
    }

    /**
     * Asserts that {@code intent} is destined for the correct component.
     */
    private static void validateIntentComponent(Intent intent) {
        assertNotNull(intent.getComponent());
        assertEquals("org.chromium.sync.notifier.TEST_VALUE",
                intent.getComponent().getClassName());
    }

    /**
     * Mock context that saves all intents given to {@code startService}.
     */
    private static class IntentSavingContext extends AdvancedMockContext {
        private final List<Intent> startedIntents = Lists.newArrayList();

        IntentSavingContext(Context targetContext) {
            super(targetContext);
        }

        @Override
        public ComponentName startService(Intent intent) {
            startedIntents.add(intent);
            return new ComponentName(this, getClass());
        }

        int getNumStartedIntents() {
            return startedIntents.size();
        }

        Intent getStartedIntent(int idx) {
            return startedIntents.get(idx);
        }

        @Override
        public PackageManager getPackageManager() {
            return getBaseContext().getPackageManager();
        }
    }
}
