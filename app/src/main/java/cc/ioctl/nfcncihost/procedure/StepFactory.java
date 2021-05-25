package cc.ioctl.nfcncihost.procedure;

import cc.ioctl.nfcncihost.procedure.step.EmptyStep;
import cc.ioctl.nfcncihost.procedure.step.LoadConfig;
import cc.ioctl.nfcncihost.procedure.step.LoadNativeLibs;

public class StepFactory {

    public static final int STEP_LOAD_NATIVE_LIBS = 10;
    public static final int STEP_LOAD_CONFIG = 12;


    public static Step getStep(int id) {
        switch (id) {
            case STEP_LOAD_NATIVE_LIBS:
                return new LoadNativeLibs();
            case STEP_LOAD_CONFIG:
                return new LoadConfig();
            default:
                return new EmptyStep();
        }
    }
}
