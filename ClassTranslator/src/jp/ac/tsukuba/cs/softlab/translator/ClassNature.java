package jp.ac.tsukuba.cs.softlab.translator;

import org.eclipse.core.resources.ICommand;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IProjectNature;
import org.eclipse.core.runtime.CoreException;

public class ClassNature implements IProjectNature {
    public static final String NATURE_ID = "ClassTranslator.ClassNature";
    private IProject project;

    public void configure() throws CoreException {
        IProjectDescription desc = project.getDescription();
        ICommand[] commands = desc.getBuildSpec();
        for (int i = 0; i < commands.length; ++i) {
            if (commands[i].getBuilderName().equals(ClassTranslator.BUILDER_ID)) {
                return;
            }
        }
        ICommand[] newCommands = new ICommand[commands.length + 1];
        System.arraycopy(commands, 0, newCommands, 0, commands.length);
        ICommand command = desc.newCommand();
        command.setBuilderName(ClassTranslator.BUILDER_ID);
        newCommands[newCommands.length - 1] = command;
        desc.setBuildSpec(newCommands);
        project.setDescription(desc, null);
    }

    public void deconfigure() throws CoreException {
        // ビルダを削除
        IProjectDescription description = getProject().getDescription();
        ICommand[] commands = description.getBuildSpec();
        for (int i = 0; i < commands.length; ++i) {
            if (commands[i].getBuilderName().equals
                (ClassTranslator.BUILDER_ID)) {
                ICommand[] newCommands = new                     ICommand[commands.length - 1];
                System.arraycopy(commands, 0, newCommands
                                                 , 0, i);
                System.arraycopy(commands, i + 1
                , newCommands, i, commands.length - i - 1);
                description.setBuildSpec(newCommands);
                return;
            }
        }
    }

    public IProject getProject() {
        return this.project;
    }

    public void setProject(IProject project) {
        this.project = project;
    }
}