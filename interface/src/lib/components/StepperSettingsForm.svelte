<script lang="ts">
	import Collapsible from '$lib/components/Collapsible.svelte';
	import Settings from '~icons/tabler/adjustments';
	import { socket } from '$lib/stores/socket';
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import Text from '$lib/components/Text.svelte';
	import type { StepperConfig, MultiStepperControl } from '$lib/types/models';

	export let stepperSettings: StepperConfig;
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
						<Checkbox
							label="Enable"
							bind:value={stepperSettings.enabled}
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
							max={100}
							bind:value={stepperSettings.maxSpeed}
							{onChange}
						></Slider>
						<Slider
							label="Max Acceleration"
							min={0}
							max={50}
							bind:value={stepperSettings.maxAccel}
							{onChange}
						></Slider>
						<Slider
							label="Driver Current"
							min={0}
							max={1000}
							bind:value={stepperSettings.driverCurrent}
							{onChange}
						></Slider>
						<Slider
							label="Steps Per Rotation"
							min={0}
							max={12000}
							step={1}
							bind:value={stepperSettings.stepsPerRot}
							{onChange}
						></Slider>
					</div>
				</div>
			</form>
		</div>
	</Collapsible>