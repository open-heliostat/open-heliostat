<script lang="ts">
	import Collapsible from '$lib/components/Collapsible.svelte';
	import Settings from '~icons/tabler/adjustments';
	import { socket } from '$lib/stores/socket';
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import Text from '$lib/components/Text.svelte';
	import type { StepperSettings, MultiStepperControl } from '$lib/types/models';

	export let stepperSettings: StepperSettings;
    export let onChange: () => void;
</script>


<style>
	.grid-form {
	  grid-template-columns: [labels] auto [controls] 1fr;
	}
</style>

	<Collapsible open={false}>
		<Settings slot="icon" class="flex-shrink-0 mr-2 h-6 w-6 self-end" />
		<span slot="title">Settings</span>
		<div class="w-full overflow-x-auto">
			<form
			>
				<div class="w-full">
					<div class="w-full grid grid-flow-row grid-form items-center">
						<Text 
							label="Name" 
							bind:value={stepperSettings.name} 
							{onChange}
						></Text>
						<Checkbox
							label="Enable on Start"
							bind:value={stepperSettings.enableOnStart}
							{onChange}
						></Checkbox>
						<Checkbox
							label="Invert Direction"
							bind:value={stepperSettings.invertDirection}
							{onChange}
						></Checkbox>
						<Slider
							label="Max Speed"
							min={0}
							max={400}
							bind:value={stepperSettings.maxSpeed}
							{onChange}
						></Slider>
						<Slider
							label="Max Acceleration"
							min={0}
							max={200}
							bind:value={stepperSettings.maxAcceleration}
							{onChange}
						></Slider>
						<Slider
							label="Driver Current"
							min={0}
							max={1000}
							step={1}
							bind:value={stepperSettings.current}
							{onChange}
						></Slider>
						<Slider
							label="Steps Per Rotation"
							min={0}
							max={4000}
							step={1}
							bind:value={stepperSettings.stepsPerRot}
							{onChange}
						></Slider>
					</div>
				</div>
			</form>
		</div>
	</Collapsible>