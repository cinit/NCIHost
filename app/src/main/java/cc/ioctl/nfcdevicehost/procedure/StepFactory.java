package cc.ioctl.nfcdevicehost.procedure;

import cc.ioctl.nfcdevicehost.procedure.step.EmptyStep;
import cc.ioctl.nfcdevicehost.procedure.step.EarlyInit;

public class StepFactory {

    public static final int STEP_EARLY_INIT = 10;

    public static Step getStep(int id) {
        switch (id) {
            case STEP_EARLY_INIT:
                return new EarlyInit();
            default:
                return new EmptyStep();
        }
    }
}
